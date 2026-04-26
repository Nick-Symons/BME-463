// Pico Zero-Dynamic-Memory Conversion of QRS_Detection.cpp

#include <math.h>
#include <string.h>
#include "functions.h"
#include "Active_Lab.h"
#include "main.h"

// ---------------------------------------------------------------------------------
// Filter coefficients / state
// ---------------------------------------------------------------------------------

#define ORDER 48
#define LOW_PASS_N 11
#define HIGH_PASS_N 32
#define DERIVATIVE_N 5
#define MWI_N 32
#define MAX_INIT 400

static float x_state[ORDER]={0};
static float x_low_pass[LOW_PASS_N]={0};
static float x_high_pass[HIGH_PASS_N]={0};
static float x_derivative[DERIVATIVE_N]={0};
static float x_mwi[MWI_N]={0};

// Low Pass Filter
static const float c_low_pass[LOW_PASS_N]={1,2,3,4,5,6,5,4,3,2,1};
static float a_low_pass = 1.0f/36.0f;          
static float x_low_pass[LOW_PASS_N] = {0};     // Input buffer

// High Pass Filter
static const float c_high_pass[HIGH_PASS_N]={-.03125,-.03125,-.03125,-.03125,-.03125,
                                              -.03125,-.03125,-.03125,-.03125,-.03125,
                                              -.03125,-.03125,-.03125,-.03125,-.03125,
                                              0.96875,-.03125,-.03125,-.03125,-.03125,
                                              -.03125,-.03125,-.03125,-.03125,-.03125,
                                              -.03125,-.03125,-.03125,-.03125,-.03125,
                                              -.03125,-.03125};
static float a_high_pass =  1/1.216;
static float x_high_pass[HIGH_PASS_N] = {0};

// Derivative Filter
static const float c_derivative[DERIVATIVE_N]={2,1,0,-1,-2};
static float a_derivative = 1/8.0;
static float x_derivative[DERIVATIVE_N] = {0};    

// Moving Window Integrator
static const float c_mwi[MWI_N]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static float a_mwi = 1/1.0;
static float x_mwi[MWI_N] = {0};

// Adaptive Threshold Detector history
#define THRESHOLD_N 3
static float x_threshold[THRESHOLD_N] = {0.0f, 0.0f, 0.0f};

// Pan-Tompkins threshold state
static float peaki = 0.0f;
static float peakt = 0.0f;
static float spki = 0.4f;          // match old Nucleo init
static float npki = 0.0f;
static float thresholdi1 = 0.15f;  // match old Nucleo init

// Heart rate detection / averaging
#define BEAT_WINDOW 14
static float rr[BEAT_WINDOW] = {0.0f};
static float timestamps[BEAT_WINDOW] = {0.0f};
static float time1 = 0.0f;     // seconds since last valid beat
float rr_avg = 0.0f;
static float hr = 0.0f;
static bool last_qrs_state = false;
bool qrs_state = false;

size_t cc = 0;
// ---------------------------------------------------------------------------------
// Adaptive threshold detector
// ---------------------------------------------------------------------------------
static void panT_Thresh(void)
{
    // Track the current local peak, update peakt as when input
    // is increasing (current sample relative to two samples prior)
    // and greater than current local peak
    if (x_threshold[0] > x_threshold[2] && x_threshold[0] > peakt) {
        peakt = x_threshold[0];
    }
    // End of candidate peak, time to update thresholds and reset local peak
    if (peakt > thresholdi1) {
        // QRS indicator is high whenever peakt > threshold
        gpio_put(QRS_LED, 1); cc = 0;
    }

    // Detect the end of a candidate peak by..
    //comparing current input to two samples prior and to peakt
    if (x_threshold[0] <= x_threshold[2] && x_threshold[0] < 0.5f * peakt) {
        peaki = peakt; 
        // Turn the QRS indicator off here
        gpio_put(QRS_LED, 0);

        // Update either spki or npki depending on if peaki is above thresholdi1
        if (peaki > thresholdi1) {
            spki = 0.125f * peaki + 0.875f * spki;
        } 
        else {
            npki = 0.125f * peaki + 0.875f * npki;
        }

        // Update thresholdi1 and reset peakt
        thresholdi1 = npki + 0.25f * (spki - npki);
        peakt = 0.0f;
    }

    // Shift threshold history exactly once ([2] gets [1], [1] gets [0])
    x_threshold[2] = x_threshold[1];
    x_threshold[1] = x_threshold[0];

    cc++;
    if (cc>500) {
        cc = 0; spki = 0; npki = 0;
        thresholdi1 = 0;
        peakt = 0;
        peaki = 0;

    }

    return;    
}

void QRS_Detection_run(float new_input_sample) {
    float new_output_sample = 0.0f;
    float low_pass_output = 0.0f;
    float high_pass_output = 0.0f;
    float derivative_output = 0.0f;
    float squaring_output = 0.0f;
    float mwi_output = 0.0f;

    // Low Pass Filter
    shift_right(x_low_pass, LOW_PASS_N);
    x_low_pass[0] = new_input_sample;
    low_pass_output = filter_FIR(
        a_low_pass,
        x_low_pass,
        c_low_pass,
        LOW_PASS_N
    );

    // High Pass Filter
    shift_right(x_high_pass, HIGH_PASS_N);
    x_high_pass[0] = low_pass_output;   
    high_pass_output = filter_FIR(
        a_high_pass,
        x_high_pass,
        c_high_pass,
        HIGH_PASS_N
    );

    // Derivative Filter

    shift_right(x_derivative, DERIVATIVE_N);
    x_derivative[0] = high_pass_output;

    derivative_output = filter_FIR(
        a_derivative,
        x_derivative,
        c_derivative,
        DERIVATIVE_N
    );

    // Squaring
    squaring_output = derivative_output * derivative_output * 500;

    // Moving Window Integration
    shift_right(x_mwi, MWI_N);
    x_mwi[0] = squaring_output;
    mwi_output = filter_FIR(
        a_mwi,
        x_mwi,
        c_mwi,
        MWI_N
    );

    // Threshold detector input is set to MWI output
    x_threshold[0] = mwi_output;

    // Call Pan Tompkins threshold function
    panT_Thresh();
}

// =========================================================
// Assigning interval times into histogram bins and calculating entropy of R-R intervals

float computeSlidingEntropy(const float rr_Intervals[BEAT_WINDOW]) {

    // Clinical AF-relevant RR bins (seconds)
    const float bins[11] = {
        0.0f, 0.3f, 0.5f, 0.6f, 0.7f,
        0.8f, 0.9f, 1.0f, 1.2f, 1.5f, 99.0f
    };

    const int BINS = 10;
    int hist[BINS] = {0};

    // Build histogram
    for (int i = 0; i < BEAT_WINDOW; i++) {

        float r = rr_Intervals[i];

        for (int b = 0; b < BINS; b++) {
            if (r >= bins[b] && r < bins[b + 1]) {
                hist[b]++;
                break;
            }
        }
    }

    // Shannon entropy (base 2)
    float entropy = 0.0f;

    for (int b = 0; b < BINS; b++) {
        if (hist[b] > 0) {
            float p = (float)hist[b] / (float)BEAT_WINDOW;
            entropy -= p * (logf(p) / logf(2.0f));
        }
    }

    return entropy;
}