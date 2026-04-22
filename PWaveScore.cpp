#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>    
#include "functions.h"

using namespace std;

// =========================================================
// Steps for P-Wave Detection:
// 1. Bandpass Filter (0.5-10 Hz)
//      1a. HPF (<0.5 Hz)
//      1b. LPF (>10 Hz)
// 2. Time alignment of QRS timestamps with original signal version (accounting for filter group delay)
//      (Will be done in the main 'detect_afib' function)
// 3. Extracting segments of the signal around each QRS timestamp to analyze for P-Wave presence
// 4. Feature extraction from segments ( just amplitude for the moment)
// =========================================================

// =========================================================
// 1. Bandpass Filter (0.5-10 Hz) 

// HPF
float x_high_pass[2] = {0};
float y_high_pass[2] = {0};
static float cx_high_pass[2] = {1.0f, -1.0f};
static float cy_high_pass[2] = {1.0f, -0.9793759f};
float a_high_pass  = 0.98968796f;

// LPF
#define LOW_PASS_N 51
float x_low_pass[LOW_PASS_N] = {0};
static float c_low_pass[LOW_PASS_N] = {0.0015218984, 0.0022943561, 0.0035908644, 0.0048960707, 0.0058952848, 
    0.0062088716, 0.0054541546, 0.0033346423, -0.00026836312, -0.0052135936, -0.011046785, -0.01699728, 
    -0.0220346, -0.02498355, -0.024687104, -0.020196246, -0.010956039, 0.0030464116, 0.021201625, 0.04228503,
    0.0645598, 0.08596756, 0.10438147, 0.11788369, 0.12502475, 0.12502475, 0.11788369, 0.10438147, 0.08596756, 
    0.0645598, 0.04228503, 0.021201625, 0.0030464116, -0.010956039, -0.020196246, -0.024687104, -0.02498355, 
    -0.0220346, -0.01699728, -0.011046785, -0.0052135936, -0.00026836312, 0.0033346423, 0.0054541546, 0.0062088716, 
    0.0058952848, 0.0048960707, 0.0035908644, 0.0022943561, 0.0015218984
};
float a_low_pass = 1.0f;  

vector<double> P_wave_BPF(const vector<double>& signal, 
                                     int windowSamples) {  // 2s at 300Hz

    // 1a. Apply high-pass filter to remove baseline wander (<0.5 Hz)
    vector<double> high_passed;
    float y = 0.0f;

    for (size_t i = 0; i < signal.size(); i++)  {
        shift_right(x_high_pass, 2);
        shift_right(y_high_pass, 2);
        x_high_pass[0] = signal[i];
        float new_output_sample = 0.0f;

        // High Pass IIR Filter
        new_output_sample = filter_IIR(
            a_high_pass,
            x_high_pass, cx_high_pass, 2,
            y_high_pass, cy_high_pass, 2
        );
        y_high_pass[0] = new_output_sample;

        // Store high passed result
        high_passed.push_back(new_output_sample);
    }

    // 1b. Apply low-pass filter to remove high-frequency noise (>10 Hz)
    vector<double> bandpassed;

    for (size_t i = 0; i < high_passed.size(); i++)  {
        shift_right(x_low_pass, LOW_PASS_N);
        x_low_pass[0] = high_passed[i];
        float new_output_sample = 0.0f;

        // Low Pass Filter
        new_output_sample = filter_FIR(
            a_low_pass,
            x_low_pass,
            c_low_pass,
            LOW_PASS_N
        );
        // Store bandpassed result
        bandpassed.push_back(new_output_sample);
    }
    // Return the bandpassed signal to be used for subsequent steps
    return bandpassed;
}

// =========================================================
// 2. Time alignment of QRS timestamps with original signal version (accounting for filter group delay)
//     (Will be done in the main 'detect_afib' function)
/* CALCULATIONS:
Both signals are now after resampling — no domain conversion needed.

QRS pipeline delay:
  Bandpass LP (11 taps)      : (11-1)/2  = 5 samples 
  Bandpass HP (32 taps)      : (32-1)/2  = 15.5 samples 
  Derivative (5 taps)        : (5-1)/2   = 2 samples 
  MWI (32 taps)              : (32-1)/2  = 15.5 samples 

  Total QRS delay    : 5 + 15.5 + 2 + 15.5 = 38 samples 

P-wave BPF delay: (excluding IIR delay which is frequency dependent and negligible for our low cutoff)
  Bandpass LP (51 taps)     : (51-1)/2  = 25 samples 

Total delay to align QRS timestamps with P-wave BPF output = 38 - 25 = 13 samples

Will be tested by overlaying detected QRS timestamps on P-Wave bandpassed signal and visually
confirming alignment with P-wave peaks

Following empirical testing, an extra 12 samples was required to achieve optimal alignment, so final NET_OFFSET is -25 samples 
This may be due to the IIR filter delay being more significant than expected at our low cutoff frequency
*/

// =========================================================
// 3. Extracting segments of the signal around each QRS timestamp to analyze for P-Wave presence
//     (Will be done in the main 'detect_afib' function)
/* CALCULATIONS

Define search window for P-wave presence as 200 ms before each QRS timestamp - 40 samples at 200Hz
    Search start = -40 samples

End of search window is set at 40 ms before QRS timestamp to avoid overlap with QRS complex - 10 samples at 200Hz
    Search end = -8 samples
*/

// =========================================================
// 4. Feature extraction from segments ( just amplitude for the moment)

vector <double> P_wave_Amplitude(const vector<double>& signal, const vector<int>& qrs_timestamps, 
                                                        int searchStart, int searchEnd) {
    vector<double> amplitudes;

    for (int ts : qrs_timestamps) {
        // Define search window around each QRS timestamp (accounting for start and end limits of the signal)
        int startIdx = max(0, ts + searchStart);
        int endIdx   = min((int)signal.size() - 1, ts + searchEnd);
        double max_amplitude = *max_element(signal.begin() + startIdx, signal.begin() + endIdx + 1);
        amplitudes.push_back(max_amplitude);
    }
    return amplitudes;
}
