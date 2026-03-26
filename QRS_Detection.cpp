#include <iostream>
#include <vector>
#include <cmath>       
#include <numeric>      
#include <algorithm>    
#include "Functions.h"

using namespace std;

// =========================================================
// Steps for QRS Detection:
// 1. Resampling to 200 Hz
// 2. Bandpass Filter (5-15 Hz)
// 3. Differentiation
// 4. Squaring
// 5. Moving Window Integration
// 6. Peak Detection
// 7. R-R Interval Calculation and Recording Timestamps to send to P-Wave Detection Function
// =========================================================

// =========================================================
// 1. Resample from 300 Hz to 200 Hz using polyphase filtering

#define NUM_SECTIONS 5
#define ORDER 3  // 2nd order → 3 taps

float x_state[NUM_SECTIONS][ORDER] = {0};
float y_state[NUM_SECTIONS][ORDER] = {0};

// Filter coefficients for resampling filter
static float cx[NUM_SECTIONS][ORDER] = {
    {1.0f, -0.49444846f, 1.0f},
    {1.0f, -0.29667230f, 1.0f},
    {1.0f,  0.16309968f, 1.0f},
    {1.0f,  0.96284880f, 1.0f},
    {1.0f,  1.84038324f, 1.0f}
};

static float cy[NUM_SECTIONS][ORDER] = {
    {1.0f, -0.88083728f, 0.83712748f},
    {1.0f, -0.67829151f, 0.56280385f},
    {1.0f, -0.45233861f, 0.32679369f},
    {1.0f, -0.23926110f, 0.13300190f},
    {1.0f, -0.10367572f, 0.01741744f}
};

static float gain[NUM_SECTIONS] = {
    0.635175999f,
    0.519284889f,
    0.404260186f,
    0.301649145f,
    0.237929827f
};

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
        float stage_in = (float)upsampled[n];
        float stage_out = 0.0f;

        // cascade through sections
        for (int s = 0; s < NUM_SECTIONS; s++)
        {
            shift_right(x_state[s], ORDER);
            x_state[s][0] = stage_in;

            stage_out = filter_IIR(
                gain[s],
                x_state[s], cx[s], ORDER,
                y_state[s], cy[s], ORDER
            );

            shift_right(y_state[s], ORDER);
            y_state[s][0] = stage_out;

            stage_in = stage_out;
        }

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

vector<double> QRS_Peaks(const vector<double>& input_signal) {
    vector<double> QRS_Peaks;
    for (size_t i = 0; i < input_signal.size(); i++)  {
        float y1 = 0.0;
        float threshold = 0.1;
        float squaring_output = 0.0;
        float MWI_output = 0.0;

        shift_right(x_derivative, DERIVATIVE_N);
        x_derivative[0] = input_signal[i];

        y1 = filter_FIR(
            a_derivative,
            x_derivative,
            c_derivative,
            DERIVATIVE_N
        );
        
        squaring_output = y1 * y1 * 500; 

        shift_right(x_mwi, MWI_N);
        x_mwi[0] = squaring_output;

        MWI_output = filter_FIR(
            a_mwi,
            x_mwi,
            c_mwi,
            MWI_N
        );

        if (MWI_output > threshold) {
            QRS_Peaks.push_back(1.0f);
            cout << "QRS Detected!" << endl;
        } 
        else {
            QRS_Peaks.push_back(0.0f);
        }
    }

    return QRS_Peaks;
}
