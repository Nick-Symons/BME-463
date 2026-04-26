
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "functions.h"
#include "Active_Lab.h"
#include "main.h"
#include <string>
using namespace std;

void detect_afib_static(double new_input_sample) {

    // =====================================================
    // STATE
    // =====================================================
    static float rr_intervals[BEAT_WINDOW] = {0.0f};
    static float timestamps[BEAT_WINDOW]   = {0.0f};

    static int beatIndex = 0;

    static float timeSinceLastBeat = 0.0f;

    float templateCorrelationMean = 0.0f;
    float templateCorrelationStd = 0.0f;

    static bool last_qrs_state = false;

    static float hr = 0.0f;
    static float rr_avg = 0.0f;

    // feature buffers
    static float entropy = 0.0f;

    // =====================================================
    // STEP 1: RUN QRS DETECTOR
    // =====================================================
    QRS_Detection_run(new_input_sample);

    bool qrs_state = (bool)gpio_get(QRS_LED);

    timeSinceLastBeat += 0.005f; // 200 Hz

    // =====================================================
    // STEP 2: DETECT RISING EDGE (new beat)
    // =====================================================
    if (qrs_state && !last_qrs_state) {

        // shift buffers
        for (int i = BEAT_WINDOW - 1; i > 0; i--) {
            rr_intervals[i] = rr_intervals[i - 1];
            timestamps[i]   = timestamps[i - 1];
        }

        // store RR interval
        rr_intervals[0] = timeSinceLastBeat;
        timestamps[0]   = timeSinceLastBeat; // could be sample index if preferred

        timeSinceLastBeat = 0.0f;

        beatIndex++;

        // =================================================
        // HEART RATE 
        // =================================================
        rr_avg = 0.0f;
        for (int i = 0; i < BEAT_WINDOW; i++) {
            rr_avg += rr_intervals[i];
        }
        rr_avg /= BEAT_WINDOW;

        if (rr_avg > 0.0f)
            hr = 60.0f / rr_avg;
        else
            hr = 0.0f;

        // Potentially can include noise 

        // =================================================
        // ONLY RUN FEATURE ENGINE EVERY 15 BEATS
        // =================================================
        if (beatIndex >= BEAT_WINDOW) {

            // reset beat counter
            beatIndex = BEAT_WINDOW;

            // =================================================
            // FEATURE 1: RR ENTROPY
            // =================================================
            float entropy = computeSlidingEntropy(rr_intervals);

            // =================================================
            // FEATURE 2: TEMPLATE DIFFERENCE 
            // =================================================
            computeTemplateCrossCorrelation(new_input_sample, timestamps[0], 60);

            // =================================================
            // FEATURE 3: KURTOSIS 
            // =================================================
            float kurtosis = computeKurtosis(new_input_sample);

            // =================================================
            // FEATURE 4: P-WAVE
            // =================================================
            float pWaveScore = 0.0f; // TODO later

            // =================================================
            // FEATURE VECTOR FOR SVM
            // =================================================
            float features[5];

            features[0] = entropy;
            features[1] = templateCorrelationMean;
            features[2] = templateCorrelationStd;
            features[3] = kurtosis;
            features[4] = pWaveScore;

            // =====================================================
            // STEP 5: SVM CLASSIFICATION (YOUR TRAINED MODEL)
            // =====================================================
            float score_normal = 0.0f;
            float score_af     = 0.0f;
            float score_noisy  = 0.0f;
            float score_other  = 0.0f;

            /*
                =====================================================
                INSERT YOUR MULTICLASS SVM HERE
                Example:
                    score_normal = dot(w0, features) + b0;
                    score_af     = dot(w1, features) + b1;
                    score_noisy  = dot(w2, features) + b2;
                    score_other  = dot(w3, features) + b3;
                =====================================================
            */

            // =====================================================
            // STEP 6: ARGMAX DECISION
            // =====================================================
            float maxScore = score_normal;
            string label = "NORMAL";

            if (score_af > maxScore) {
                maxScore = score_af;
                label = "AF";
            }
            if (score_noisy > maxScore) {
                maxScore = score_noisy;
                label = "NOISY";
            }
            if (score_other > maxScore) {
                maxScore = score_other;
                label = "OTHER";
            }
            printf("Signal Type: %s\n", label.c_str());
            printf("Features: Entropy=%.3f, TemplateCorrMean=%.3f, TemplateCorrStd=%.3f, Kurtosis=%.3f, PWaveScore=%.3f\n",
                   entropy, templateCorrelationMean, templateCorrelationStd, kurtosis, pWaveScore);
        }
}