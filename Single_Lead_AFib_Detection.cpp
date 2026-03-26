#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem; 

// =========================================================
// 1. THE ALGORITHM
// =========================================================

string detect_afib(const vector<double>& signal) {
    
    // --- STEP 1: VARIANCE CALCULATION (Noise Gate) ---
    // If (Variance > Threshold) -> Noisy
    
    // --- STEP 2: VENTRICULAR RESPONSE (Pan-Tompkins) ---
    // Calculate R-R Intervals and Entropy
    
    // --- STEP 3: ATRIAL ACTIVITY (P-Wave) ---
    // Check P-Wave presence/amplitude
    
    // --- STEP 4: CLASSIFICATION LOGIC ---
    // if (entropy > HIGH && pwave < LOW) return "AF";
    
    return "normal"; // Placeholder result
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

    for (const auto& folder : fs::directory_iterator(root_path)) {
        if (folder.is_directory()) {
            
            // Here is your 'signal_label' variable
            // It captures "AF", "Normal", or "Noisy" from the folder name
            string signal_label = folder.path().filename().string();
            
            cout << "\nChecking Category: [" << signal_label << "]" << endl;

            for (const auto& file : fs::directory_iterator(folder.path())) {
                
                // 1. ACQUISITION
                vector<double> signal_data = acquire_from_disk(file.path().string());
                
                if (signal_data.empty()) continue;

                // 2. RUN THE ALGORITHM
                // We send the raw data into the 'Brain'
                string prediction = detect_afib(signal_data);

                // 3. COMPARE & LOG
                // You can now compare 'prediction' against 'signal_label' easily
                if (prediction == signal_label) {
                    cout << "  [PASS] File: " << file.path().filename() << endl;
                } else {
                    cout << "  [FAIL] File: " << file.path().filename() 
                         << " (Predicted: " << prediction << ")" << endl;
                }
            }
        }
    }
}

int main() {
    // Replace with your actual folder name
    run_batch_training("C:\\Users\\timmy\\OneDrive\\TCD Engineering\\SS - Semester 2\\Computers in Medicine\\Project - Single Lead AF Classification\\output"); 
    return 0;
}
