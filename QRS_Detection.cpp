#include <iostream>
#include <vector>
#include <cmath>       
#include <numeric>      
#include <algorithm>    
#include "functions.h"

using namespace std;

// Currently using dynamic allocation for the assignment of variables
// Will need to switch to static allocation for the Raspberry Pi Pico implementation,
// but this is more flexible for testing and debugging on PC during development phase

// =========================================================
// Steps for QRS Detection:
// 1. Resampling to 200 Hz
// 2. Bandpass Filter (5-15 Hz)
// 3. Differentiation
// 4. Squaring
// 5. Moving Window Integration
// 6. Peak Detection
// 7. R-R Interval Calculation and Recording Timestamps to send to P-Wave Detection Function
// 8. Assigning interval times into histogram bins and calculating entropy of R-R intervals
// =========================================================

// =========================================================
// 1. Resample from 300 Hz to 200 Hz using polyphase filtering

#define ORDER 48  // 2nd order → 3 taps

float x_state[ORDER] = {0};

// Filter coefficients for resampling filter
static float cx[ORDER] = {-0.001621, -0.001222, 0.002827, 0.010480, 0.016633, 0.014543, 0.002976,
     -0.009870, -0.012021, 0.000086, 0.015180, 0.016004, -0.002277, -0.023029, -0.021805, 0.006816,
     0.036836, 0.031457, -0.017088, -0.067212, -0.054252, 0.052347, 0.210995, 0.328564, 0.328564, 
     0.210995, 0.052347, -0.054252, -0.067212, -0.017088, 0.031457, 0.036836, 0.006816, -0.021805,
     -0.023029, -0.002277, 0.016004, 0.015180, 0.000086, -0.012021, -0.009870, 0.002976, 0.014543, 
     0.016633, 0.010480, 0.002827, -0.001222, -0.001621
};

float LPF_Gain = 2.0f;  

vector<double> resampled_signal(const vector<double>& input_signal) {
    int upsample_factor = 2;
    int downsample_factor = 3;

    // -------- Upsample --------
    vector<double> upsampled;
    for (size_t i = 0; i < input_signal.size(); i++) {
        upsampled.push_back(input_signal[i]);
        upsampled.push_back(0.0);
    }

    // -------- Filter --------
    vector<double> filtered;

    for (size_t n = 0; n < upsampled.size(); n++)  
    {
        float new_input_sample = upsampled[n];
        float stage_out = 0.0f;
        shift_right(x_state, ORDER);
        x_state[0] = new_input_sample;

        stage_out = filter_FIR(
            LPF_Gain,
            x_state,
            cx,
            ORDER
        );

        filtered.push_back(stage_out);  
    }

    // -------- Downsample --------
    vector<double> downsampled;
    for (size_t i = 0; i < filtered.size(); i += downsample_factor) {
        downsampled.push_back(filtered[i]);
    }
    // Return the downsampled signal to be used for subsequent steps
    return downsampled;
}

// =========================================================
// 2. Bandpass Filter (5-15 Hz) - Implemented as a cascade of a low-pass and high-pass filter

// Low Pass Filter
#define LOW_PASS_N  11
static float c_low_pass[LOW_PASS_N] = {1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1};   // Coefficients
static float a_low_pass = 1.0f/36.0f;          
static float x_low_pass[LOW_PASS_N] = {0};     // Input buffer


// High Pass Filter
#define HIGH_PASS_N  32
static float c_high_pass[HIGH_PASS_N] = {-.03125, -.03125, -.03125, -.03125, -.03125, -.03125, -.03125, -.03125, -.03125,
                                        -.03125, -.03125, -.03125, -.03125, -.03125, -.03125, 0.96875, -.03125, -.03125,
                                        -.03125, -.03125, -.03125, -.03125, -.03125, -.03125, -.03125, -.03125, -.03125,
                                        -.03125, -.03125, -.03125, -.03125, -.03125};
static float a_high_pass =  1/1.216f;
static float x_high_pass[HIGH_PASS_N] = {0};

vector<double> bandpassed_signal(const vector<double>& input_signal) {
    vector<double> bandpassed;
    for (size_t i = 0; i < input_signal.size(); i++)  {
        shift_right(x_low_pass, LOW_PASS_N);
        x_low_pass[0] = input_signal[i];
        float y1;
        float new_output_sample = 0.0f;

        // Low Pass Filter
        y1 = filter_FIR(
            a_low_pass,
            x_low_pass,
            c_low_pass,
            LOW_PASS_N
        );

        shift_right(x_high_pass, HIGH_PASS_N);
        x_high_pass[0] = y1;

        // High Pass Filter
        new_output_sample = filter_FIR(
            a_high_pass,
            x_high_pass,
            c_high_pass,
            HIGH_PASS_N
        );
        // Store bandpassed result
        bandpassed.push_back(new_output_sample);
    }
    // Return the bandpassed signal to be used for subsequent steps
    return bandpassed; 
}

// =========================================================
// 3-6. Differentiation, Squaring, and Moving Window Integration

#define DERIVATIVE_N  5
static float c_derivative[DERIVATIVE_N] = {2, 1, 0, -1, -2};
static float a_derivative = 1/8.0;
static float x_derivative[DERIVATIVE_N] = {0};    


#define MWI_N  32
static float c_mwi[MWI_N] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static float a_mwi = 1/1.0;
static float x_mwi[MWI_N] = {0};

vector<double> QRS_Peaks(const vector<double>& input_signal){
    vector<double> QRS_Peaks;

    // ── Adaptive threshold state — must be outside the loop ──────────────────
    float SPKI       = 0.0f;
    float NPKI       = 0.0f;
    float THRESHOLD1 = 0.0f;
    float THRESHOLD2 = 0.0f;
    float samples_since_last_peak = 1000; 

    // ── Local max tracking — outside the loop ─────────────────────────────────
    float localMax    = 0.0f;
    int   localMaxIdx = -1;
    bool  inPeak      = false;

    // Pre-pass to initialise thresholds from first 2 seconds
    vector<float> mwiBuffer;
    for (size_t i = 0; i < input_signal.size(); i++) {
        shift_right(x_derivative, DERIVATIVE_N);
        x_derivative[0] = input_signal[i];
        float y1 = filter_FIR(a_derivative, x_derivative, c_derivative, DERIVATIVE_N);
        float squaring_output = y1 * y1 * 500;
        shift_right(x_mwi, MWI_N);
        x_mwi[0] = squaring_output;
        mwiBuffer.push_back(filter_FIR(a_mwi, x_mwi, c_mwi, MWI_N));
    }

    int   initSamples = min((int)(2.0 * 200.0), (int)mwiBuffer.size());
    float maxInit     = *max_element(mwiBuffer.begin(), mwiBuffer.begin() + initSamples);
    SPKI              = 0.25f  * maxInit;
    NPKI              = 0.125f * maxInit;
    THRESHOLD1        = NPKI + 0.25f * (SPKI - NPKI);
    THRESHOLD2        = 0.5f  * THRESHOLD1;

    // Reset filter states for second pass
    fill(x_derivative, x_derivative + DERIVATIVE_N, 0.0f);
    fill(x_mwi,        x_mwi        + MWI_N,        0.0f);

    // ── Main detection loop ───────────────────────────────────────────────────
    for (size_t i = 0; i < input_signal.size(); i++) {
        float y1              = 0.0f;
        float squaring_output = 0.0f;
        float MWI_output      = 0.0f;

        shift_right(x_derivative, DERIVATIVE_N);
        x_derivative[0] = input_signal[i];
        y1 = filter_FIR(a_derivative, x_derivative, c_derivative, DERIVATIVE_N);

        squaring_output = y1 * y1 * 500;

        shift_right(x_mwi, MWI_N);
        x_mwi[0] = squaring_output;
        MWI_output = filter_FIR(a_mwi, x_mwi, c_mwi, MWI_N);

        samples_since_last_peak++;

        // Track rising/falling to find local maximum
        if (MWI_output > localMax) {
            localMax    = MWI_output;
            localMaxIdx = i;
            inPeak      = true;
        }

        // Only make a detection decision when we've passed the peak (signal starts falling)
        if (inPeak && MWI_output < localMax * 0.9f) {
            inPeak = false;

            if (localMax >= THRESHOLD1 && samples_since_last_peak > 40) {
                // Mark the fiducial point at the local max index
                QRS_Peaks.push_back(1.0f);
                samples_since_last_peak = 0;
                localMax    = 0.0f;
                SPKI        = 0.125f * localMax + 0.875f * SPKI;
            } else {
                QRS_Peaks.push_back(0.0f);
                NPKI        = 0.125f * localMax + 0.875f * NPKI;
                localMax    = 0.0f;
            }

            THRESHOLD1 = NPKI + 0.25f * (SPKI - NPKI);
            THRESHOLD2 = 0.5f * THRESHOLD1;
        } else {
            QRS_Peaks.push_back(0.0f);
        }
    }

    return QRS_Peaks;
}

// =========================================================
// 7. Extract QRS timestamps and calculate R-R intervals (to be sent to P-Wave Detection Function)

// Detects rising edges (0→1 transitions) in the pulse vector.
// Returns sample indices of each QRS onset — use these as your timestamps.
vector<int> extractQRSTimestamps(const vector<double>& pulseVec, double threshold) {
    vector<int> timestamps;

    for (int i = 1; i < (int)pulseVec.size(); i++) {
        bool prevLow  = pulseVec[i - 1] < threshold;
        bool currHigh = pulseVec[i]     >= threshold;
        if (prevLow && currHigh) {
            timestamps.push_back(i);  // rising edge index
        }
    }
    return timestamps;
}
// Timestamps are currently for 200Hz downsampled signal version
// Need to multiply by 1.5 to convert back to 300Hz original signal version before sending to P-Wave Detection Function
// Also need to account for group delay from filters (e.g. 5 samples for low-pass FIR) to align timestamps with original signal version

// Converts consecutive QRS timestamp pairs into interval lengths (in samples).
// Divide by fs (e.g. 360.0) to get seconds.
vector<int> computeRRIntervals(const vector<int>& timestamps) {

    vector<int> rrIntervals;
    rrIntervals.reserve(timestamps.size() - 1);

    for (int i = 1; i < (int)timestamps.size(); i++) {
        rrIntervals.push_back(timestamps[i] - timestamps[i - 1]);
    }
    return rrIntervals;
}

// =========================================================
// 8. Assigning interval times into histogram bins and calculating entropy of R-R intervals

vector<double> computeSlidingEntropy(const vector<int>& rrIntervals, double fs,
                                     int windowSize) {
    // Bin edges in seconds
    // Fine resolution in the clinically relevant range for AF (0.3–1.2s)
    const vector<double> binEdges = {0.0, 0.3, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.2, 1.5, 99.0};

    vector<double> entropyValues;

    for (int i = windowSize; i <= (int)rrIntervals.size(); i++) {

        // Count into bins
        vector<int> histogram(binEdges.size() - 1, 0);

        // 10 beat sliding window of R-R intervals
        for (int j = i - windowSize; j < i; j++) {
            double rrSec = rrIntervals[j] / fs;
            for (int b = 0; b < (int)histogram.size(); b++) {
                if (rrSec >= binEdges[b] && rrSec < binEdges[b + 1]) {
                    histogram[b]++;
                    break;
                }
            }
        }

        // Shannon entropy
        double entropy = 0.0;
        for (int count : histogram) {
            double p = static_cast<double>(count) / windowSize;
            if (p > 0.0)
                entropy -= p * log2(p);
        }
        entropyValues.push_back(entropy);
    }
    return entropyValues;
}