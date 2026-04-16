#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <numeric>
#include <algorithm>
#include "functions.h"

using namespace std;
namespace fs = std::filesystem; 

void dumpCSV(const string& filename, const vector<double>& signal) {
    ofstream f(filename);
    for (int i = 0; i < (int)signal.size(); i++)
        f << i << "," << signal[i] << "\n";
}

// Currently using dynamic allocation for the assignment of variables
// Will need to switch to static allocation for the Raspberry Pi Pico implementation,
// but this is more flexible for testing and debugging on PC during development phase

// =========================================================
// 1. THE ALGORITHM (PORTABLE "BRAIN")
// =========================================================
// This function is what you will eventually copy-paste 
// onto your Raspberry Pi Pico.
void detect_afib(const vector<double>& signal, 
                 const string&         true_label,
                 const string&         filename,
                 ofstream&             results_csv) {
    
    /* REMOVING SEPERATE NOISE CLASSIFICATION STAGE - results were not good and noise and signal power indistinguishable 
    in ECG records due to single lead detection, so just going to rely on ventricular response and atrial activity features
    for classification instead

    // --- STEP 1: VARIANCE CALCULATION (Noise Gate) ---
    // According to your flowchart: If (Variance > Threshold) -> Noisy
    vector<double> kurtValues = computeKurtosis(signal);
    double meanKurt = accumulate(kurtValues.begin(), kurtValues.end(), 0.0) / kurtValues.size();   
    vector<double> residualValues = computeHF_Residual(signal); 
    double meanRes = accumulate(residualValues.begin(), residualValues.end(), 0.0) / residualValues.size();   
    */

    // --- STEP 2: VENTRICULAR RESPONSE (Pan-Tompkins) ---
    // Calculate R-R Intervals and Entropy
    vector<double> resampled = resampled_signal(signal);
    vector<double> bandpassed = bandpassed_signal(resampled);
    vector<double> qrs_peaks = QRS_Peaks(bandpassed);
    vector<int> timestamps = extractQRSTimestamps(qrs_peaks);

    // Dump every stage to CSV for plotting
    dumpCSV("_1_raw.csv",        signal);
    dumpCSV("_2_resampled.csv",  resampled);
    dumpCSV("_3_bandpassed.csv", bandpassed);
    dumpCSV("_4_qrs_peaks.csv",  qrs_peaks);

    if (timestamps.size() < 2) {
        results_csv << filename << "," << true_label << ",NA,NA,NA\n";
        return;
    }

    // Noise Guard 1: implausible beat count → noisy
    double recordingSec = signal.size() / 300.0;
    double bpm = (timestamps.size() / recordingSec) * 60.0;
    printf("Recording: %s, BPM: %.2f\n", filename.c_str(), bpm);
    if (bpm < 20 || bpm > 220) {
        results_csv << filename << "," << true_label << ",NA,NA,NA\n";
        return;
    }

    // Noise Guard 2: too few intervals for reliable entropy → noisy
    vector<int> intervals = computeRRIntervals(timestamps);
    printf("Recording: %s, R-R Intervals: %d\n", filename.c_str(), (int)intervals.size());
    if (intervals.size() < 10) {
        results_csv << filename << "," << true_label << ",NA,NA,NA\n";
        return;
    }

    vector<double> entropy = computeSlidingEntropy(intervals);

    if (entropy.empty()) {
        results_csv << filename << "," << true_label << ",NA,NA,NA\n";
        return;
    }

    // Compute summary statistics over all windows in this recording
    double meanEnt = accumulate(entropy.begin(), entropy.end(), 0.0) / entropy.size();
    double maxEnt  = *max_element(entropy.begin(), entropy.end());
    double minEnt  = *min_element(entropy.begin(), entropy.end());

    results_csv << filename    << ","
                << true_label  << ","
                << meanEnt     << ","
                << maxEnt      << ","
                << minEnt      << "\n";

    // --- STEP 3: ATRIAL ACTIVITY (P-Wave) ---
    // Check P-Wave presence/amplitude
    
    // --- STEP 4: CLASSIFICATION LOGIC ---
    // if (entropy > HIGH && pwave < LOW) return "AF";
    
    // return "normal"; // Placeholder result
}

// =========================================================
// 2. DATA ACQUISITION (PC TRAINING ONLY)
// =========================================================
vector<double> acquire_from_disk(string file_path) {
    vector<double> buffer;
    ifstream file(file_path);
    double val;
    while (file >> val) {
        buffer.push_back(val);
    }
    return buffer;
}

// =========================================================
// 3. TRAINING / BATCH PROCESSING LOOP
// =========================================================
void run_batch_training(string root_path) {
    // Check if the path exists to avoid crashes
    if (!fs::exists(root_path)) {
        cout << "Error: Folder " << root_path << " not found!" << endl;
        return;
    }

    ofstream results("results.csv");
    results << "filename,true_label,mean_entropy,max_entropy,min_entropy\n";

    for (const auto& folder : fs::directory_iterator(root_path)) {
        if (folder.is_directory()) {
            
            // Here is your 'signal_label' variable
            // It captures "AF", "Normal", or "Noisy" from the folder name
            string signal_label = folder.path().filename().string();
            
            cout << "\nChecking Category: [" << signal_label << "]" << endl;

            for (const auto& file : fs::directory_iterator(folder.path())) {
                
                // 1. ACQUISITION
                vector<double> signal_data = acquire_from_disk(file.path().string());
                // Normalise data
                for (int i = 0; i < signal_data.size(); i++) {
                    signal_data[i] = signal_data[i]/4096 - 0.5;
                }
                
                if (signal_data.empty()) continue;

                // 2. RUN THE ALGORITHM
                // We send the raw data into the 'Brain'
                detect_afib(signal_data,
                            signal_label,
                            file.path().stem().string(),
                            results);
                
                cout << "Results written to results.csv" << endl;

                /*
                string prediction = "normal"; // Placeholder prediction
                // 3. COMPARE & LOG
                // You can now compare 'prediction' against 'signal_label' easily
                if (prediction == signal_label) {
                    cout << "  [PASS] File: " << file.path().filename() << endl;
                } else {
                    cout << "  [FAIL] File: " << file.path().filename() 
                         << " (Predicted: " << prediction << ")" << endl;
                }
                */
            }
        }
    }
    results.flush();
    results.close();
}

int main() {
    // Replace with your actual folder name
    run_batch_training("C:\\Users\\timmy\\OneDrive\\TCD Engineering\\SS - Semester 2\\Computers in Medicine\\Project - Single Lead AF Classification\\output"); 
    return 0;
}
