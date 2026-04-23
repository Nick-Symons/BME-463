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
    */
    // Bringing it back for the one time cmon

    // --- STEP 1: VARIANCE CALCULATION (Noise Gate) ---
    // According to your flowchart: If (Variance > Threshold) -> Noisy
    vector<double> kurtValues = computeKurtosis(signal);
    double meanKurtosis = accumulate(kurtValues.begin(), kurtValues.end(), 0.0) / kurtValues.size();   
    // vector<double> residualValues = computeHF_Residual(signal); 
    // double meanResidual = accumulate(residualValues.begin(), residualValues.end(), 0.0) / residualValues.size();   

    // --- STEP 2: VENTRICULAR RESPONSE (Pan-Tompkins) ---
    // Calculate R-R Intervals and Entropy
    vector<double> resampled = resampled_signal(signal);
    vector<double> bandpassed = bandpassed_signal(resampled);
    vector<double> qrs_peaks = QRS_Peaks(bandpassed);
    vector<int> timestamps = extractQRSTimestamps(qrs_peaks);

    if (timestamps.size() < 2) {
        results_csv << filename << "," << true_label << ",NA,NA,NA,NA,NA\n";
        return;
    }

    // *Didn't* do anything useful and just adds complexity to the algorithm, so removing for now
    // Let's see how an SVM will fuck with it

    // Noise Guard 1: implausible beat count → noisy
    double recordingSec = signal.size() / 300.0;
    double bpm = (timestamps.size() / recordingSec) * 60.0;
    // printf("Recording: %s, BPM: %.2f\n", filename.c_str(), bpm);
    if (bpm < 20 || bpm > 220) {
        results_csv << filename << "," << true_label << ",NA,NA,NA,NA,NA\n";
        return;
    }
    
    // Noise Guard 2: too few intervals for reliable entropy → noisy
    vector<int> intervals = computeRRIntervals(timestamps);
    // printf("Recording: %s, R-R Intervals: %d\n", filename.c_str(), (int)intervals.size());
    if (intervals.size() < 8) {
        results_csv << filename << "," << true_label << ",NA,NA,NA,NA,NA\n";
        return;
    }
    
    vector<double> templateOut;
    vector<double> ssdPerBeat;
    vector<double> diffSignal;

    vector<double> entropy = computeSlidingEntropy(intervals);
    double RMSSD = computeRMSSD(intervals);
    double templateDiff = computeTemplateDifference(resampled, timestamps, templateOut, diffSignal);

    if (entropy.empty()) {
        results_csv << filename << "," << true_label << ",NA,NA,NA,NA,NA\n";
        return;
    }

    // --- STEP 3: ATRIAL ACTIVITY (P-Wave) ---
    // Check P-Wave presence/amplitude
    vector<double> PWave_BPF_signal = P_wave_BPF(resampled);

    // Align QRS timestamps with P-wave BPF output (accounting for filter delays)
    int NET_OFFSET = -22;          // 11ms at 200hz

    vector<int> qrs_in_Pwave;
    qrs_in_Pwave.reserve(timestamps.size());

    for (int ts : timestamps) {
        int timestamp = ts + NET_OFFSET;
        qrs_in_Pwave.push_back(timestamp);
    }

    int searchStart = -35; int searchEnd = -12; 
    vector<int> searchRegion;
    searchRegion.reserve(2*timestamps.size());
    for (int ts : qrs_in_Pwave) {
        // Define search window around each QRS timestamp (accounting for start and end limits of the signal)
        int startIdx = max(0, ts + searchStart);
        int endIdx   = min((int)signal.size() - 1, ts + searchEnd);
        searchRegion.push_back(startIdx);
        searchRegion.push_back(endIdx);
    }

    // Extract segments around each QRS timestamp and compute features for P-wave presence
    vector<double> p_wave_amplitudes = P_wave_Amplitude(PWave_BPF_signal, qrs_in_Pwave);

    // --- STEP 4: LOGGING & VISUALISATION ---

    // Compute summary statistics over all windows in this recording
    double meanEnt = accumulate(entropy.begin(), entropy.end(), 0.0) / entropy.size();
    double maxEnt  = *max_element(entropy.begin(), entropy.end());
    double minEnt  = *min_element(entropy.begin(), entropy.end());
    double meanPWaveAmp = accumulate(p_wave_amplitudes.begin(), p_wave_amplitudes.end(), 0.0) / p_wave_amplitudes.size();

    // Convert qrs_in_Pwave to double for CSV dumping
    vector<double> qrs_timestamps(qrs_in_Pwave.begin(), qrs_in_Pwave.end());
    vector<double> search_region_double(searchRegion.begin(), searchRegion.end());

    // Dump every stage to CSV for plotting
    dumpCSV("_1_raw.csv",           signal);
    dumpCSV("_2_qrs_peaks.csv",     qrs_peaks);
    dumpCSV("_3_PWave_BPF.csv",     PWave_BPF_signal);
    dumpCSV("_4_QRStimestamps.csv", qrs_timestamps);
    dumpCSV("_5_PWaveAmps.csv",     p_wave_amplitudes);
    dumpCSV("_6_SearchRegions.csv", search_region_double);
    dumpCSV("_7_template.csv",     templateOut);
    dumpCSV("_8_diff_signal.csv",  diffSignal);

    // Log summary statistics to results.csv for tuning of thresholds and classification logic
    results_csv << filename     << ","
                << true_label   << ","
                << meanKurtosis << ","
                << meanEnt      << ","
                << RMSSD        << ","
                << templateDiff << ","
                << meanPWaveAmp << "\n";

    // --- STEP 5: CLASSIFICATION LOGIC ---
    // For now, just logging features to CSV and tuning thresholds in Python, 
    // but this is where we will implement the actual classification logic based 
    // on the features we have extracted.

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
    results << "filename,true_label,mean_kurtosis,mean_entropy,mean_RMSSD,mean_template_diff,mean_p_wave_amplitude\n";

    for (const auto& folder : fs::directory_iterator(root_path)) {
        if (folder.is_directory()) {
            
            // Here is your 'signal_label' variable
            // It captures "AF", "Normal", or "Noisy" from the folder name
            string signal_label = folder.path().filename().string();

            /*
            if (signal_label == "noisy") {
                cout << "\n[" << signal_label << "] Skipping this folder." << endl;
                continue;
            }
            */

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
               // return; // Remove this return statement to process all files in the folder, currently just processing one file for testing
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
