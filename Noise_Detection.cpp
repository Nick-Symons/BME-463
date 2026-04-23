// Not working well for classification, but keeping here for reference
// Noise and signal power indistinguishable in ECG records due to single lead detection

#include <iostream>
#include <vector>
#include <cmath>       
#include <numeric>      
#include <algorithm>    
#include "functions.h"

using namespace std;

// =========================================================
// Steps for noise detection
// 1. Calculate kurtosis over 2s interval
// 2. Use for classification
// =========================================================

vector<double> computeKurtosis(const vector<double>& inputSignal) {

    vector<double> kurtosisValues;
    int windowCounter = 0;
    const int windowSize = 400;  // 2s at 200Hz
    double windowBuffer[windowSize] = {0};

    for (size_t i = 0; i < inputSignal.size(); i++) {
        windowBuffer[windowCounter] = inputSignal[i];
        windowCounter++;

        if (windowCounter >= windowSize) {
            double mean = 0.0;
            for (int j = 0; j < windowSize; j++) mean += windowBuffer[j];
            mean /= windowSize;

            double var  = 0.0;
            double kurt = 0.0;
            for (int j = 0; j < windowSize; j++) {
                double diff  = windowBuffer[j] - mean;
                var         += diff * diff;
                kurt        += diff * diff * diff * diff;
            }
            var  /= windowSize;
            kurt /= windowSize;

            double kurtosis = (var > 0) ? kurt / (var * var) : 0.0;
            kurtosisValues.push_back(kurtosis);
            windowCounter = 0;
        }
    }
    return kurtosisValues;
}

// Want to establish how much high frequency content there is in a signal, i.e. how quickly the signal changes
// Two potential approaches: 
// 1) Apply simple LPF and calculate the magnitude of the residual (difference between original and filtered signals)
// 2) Continuously add up the difference between adjacent samples to calculate a value representing how fast the signal changes
/*
// Going to use option 2 for the moment as easier to compute and don't have to worry about filter group delay and alignment of
// residual with original signal version, but can switch to option 1 if this doesn't work well for classification
vector<double> computeHF_Residual(const vector<double>& inputSignal) {

    // Have to compute over fixed window
    vector<double> residualValues;
    int windowCounter = 0;
    const int windowSize = 400;  // 2s at 200Hz
    double windowBuffer[windowSize] = {0};

    for (size_t i = 0; i < inputSignal.size(); i++) {
        windowBuffer[windowCounter] = inputSignal[i];
        windowCounter++;

        if (windowCounter >= windowSize) {
            double residual = 0;
            for (int j = 1; j < windowSize; j++) {
                residual += pow((windowBuffer[j] - windowBuffer[j-1]), 2);
            }
            
            residualValues.push_back(residual);
            windowCounter = 0;
        }
    }
    return residualValues;
}
*/ 

// Option 1 - Apply simple LPF and calculate the magnitude of the residual (difference between original and filtered signals)

// High Pass Filter
#define HIGH_PASS_N  41
static float c_high_pass[HIGH_PASS_N] = {
  0.0138121f, -0.00978287f, -0.00948737f, -0.00910602f,
 -0.00700129f, -0.00225793f,  0.00500932f,  0.0135511f,
  0.0211221f,  0.0249158f,   0.0221847f,   0.0110238f,
 -0.00912934f, -0.0369226f, -0.0693216f,  -0.101989f,
 -0.130009f,  -0.148919f,   0.844406f,   -0.148919f,
 -0.130009f,  -0.101989f,  -0.0693216f,  -0.0369226f,
 -0.00912934f,  0.0110238f,  0.0221847f,   0.0249158f,
  0.0211221f,  0.0135511f,  0.00500932f, -0.00225793f,
 -0.00700129f, -0.00910602f, -0.00948737f, -0.00978287f,
  0.0138121f
};static float a_high_pass = 1.0f;          
static float x_high_pass[HIGH_PASS_N] = {0};     // Input buffer

vector<double> computeHF_Residual(const vector<double>& inputSignal) {
    vector<double> residualValues;

    for (size_t i = 0; i < inputSignal.size(); i++) {
        shift_right(x_high_pass, HIGH_PASS_N);
        x_high_pass[0] = inputSignal[i];

        float y = filter_FIR(
            a_high_pass,     
            x_high_pass,
            c_high_pass,
            HIGH_PASS_N
        );

        // Square to get magnitude (energy)
        residualValues.push_back(y * y);
    }

    return residualValues;
}

// =========================================================

double computeTemplateDifference(const vector<double>& signal,
                      const vector<int>&    timestamps,
                      vector<double>&       templateOut,
                      vector<double>&       diffSignal,
                      int                   halfWin) {
    constexpr int QRS_PIPELINE_DELAY = 38;  // samples at 200Hz
    // Adjust timestamps for pipeline delay to align with original signal version
    if ((int)timestamps.size() < 5) return 0.0;

    // Build template from beats 2-4
    vector<double> templateBeat(halfWin * 2, 0.0);
    int templateCount = 0;

    for (int i = 2; i <= 5 && i < (int)timestamps.size(); i++) {
        int start = timestamps[i] - halfWin - QRS_PIPELINE_DELAY;
        if (start < 0 || start + halfWin * 2 >= (int)signal.size())
            continue;
        for (int j = 0; j < halfWin * 2; j++)
            templateBeat[j] += signal[start + j];
        templateCount++;
    }
    if (templateCount == 0) return 0.0;
    for (double& v : templateBeat) v /= templateCount;

    // Expose template for plotting
    templateOut = templateBeat;

    // Initialise difference signal to zero - same length as input signal
    // Primarily used for plotting
    diffSignal.assign(signal.size(), 0.0);

    // SSD over all beats compared to template
    double totalSSD = 0.0;
    int    ssdCount = 0;
    double mean = 0.0;
    double M2   = 0.0;

    for (int i = 6; i < (int)timestamps.size(); i++) {
        int start = timestamps[i] - halfWin - QRS_PIPELINE_DELAY;

        // Only compute if we have a full window of signal to compare against the template
        if (start < 0 || start + halfWin * 2 > (int)signal.size() - 1)
            continue;

        double ssd = 0.0;
        for (int j = 0; j < halfWin * 2; j++) {
            double diff = signal[start + j] - templateBeat[j];
            diffSignal[start + j] = diff;  // write raw difference into signal position
            ssd        += diff * diff;
        }
        totalSSD += ssd / (halfWin * 2);
        ssdCount++;

        // ---- Welford update ----
        double delta = ssd - mean;
        mean += delta / ssdCount;
        double delta2 = ssd - mean;
        M2 += delta * delta2;

    }
    return (ssdCount > 0) ? totalSSD / ssdCount : 0.0;
}
