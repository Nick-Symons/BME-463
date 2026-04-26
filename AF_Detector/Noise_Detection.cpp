
#include <math.h>
#include <string.h>
#include "functions.h"
#include "Active_Lab.h"
#include "main.h"

#define NOISE_WINDOW 400
#define HF_N 41

static float x_hf[HF_N]={0};
static const float c_hf[HF_N]={
0.0138121f,-0.00978287f,-0.00948737f,-0.00910602f,
-0.00700129f,-0.00225793f,0.00500932f,0.0135511f,
0.0211221f,0.0249158f,0.0221847f,0.0110238f,
-0.00912934f,-0.0369226f,-0.0693216f,-0.101989f,
-0.130009f,-0.148919f,0.844406f,-0.148919f,
-0.130009f,-0.101989f,-0.0693216f,-0.0369226f,
-0.00912934f,0.0110238f,0.0221847f,0.0249158f,
0.0211221f,0.0135511f,0.00500932f,-0.00225793f,
-0.00700129f,-0.00910602f,-0.00948737f,-0.00978287f,
0.0138121f};

double computeKurtosis(double newSample) {

    const int windowSize = 3000;  // 15s at 200Hz

    // Persistent state (keeps values across calls)
    static int windowCounter = 0;

    static double sum1 = 0.0;
    static double sum2 = 0.0;
    static double sum3 = 0.0;
    static double sum4 = 0.0;

    static double lastKurtosis = 0.0;

    double x = newSample;
    double x2 = x * x;

    // Update running sums
    sum1 += x;
    sum2 += x2;
    sum3 += x2 * x;
    sum4 += x2 * x2;

    windowCounter++;

    if (windowCounter >= windowSize) {

        double mean = sum1 / windowSize;

        double ex2 = sum2 / windowSize;
        double ex3 = sum3 / windowSize;
        double ex4 = sum4 / windowSize;

        double var = ex2 - (mean * mean);

        double kurtosis = 0.0;

        if (var > 0.0) {
            double mu4 =
                ex4
                - 4.0 * mean * ex3
                + 6.0 * mean * mean * ex2
                - 3.0 * mean * mean * mean * mean;

            kurtosis = mu4 / (var * var);
        }

        // store result
        lastKurtosis = kurtosis;

        // reset window
        windowCounter = 0;
        sum1 = sum2 = sum3 = sum4 = 0.0;
    }

    return lastKurtosis;
}

// =========================================================
// Template Matching Cross-Correlation for Noise Detection
// =========================================================

void computeTemplateCrossCorrelation(double signalSample,
                                     int qrsTimestamp,
                                     int halfWin) {

    constexpr int QRS_PIPELINE_DELAY = 38;

    // -----------------------------
    // Persistent state
    // -----------------------------
    static int beatIndex = 0;

    static double templateBeat[1000];
    static double sumTemplate[1000];
    static int templateCount = 0;
    static bool templateReady = false;

    static double corrMean = 0.0;
    static double corrM2 = 0.0;
    static int corrCount = 0;

    static double beatBuffer[1000];

    int winLen = halfWin * 2;

    // -----------------------------
    // reset safety
    // -----------------------------
    if (beatIndex == 0) {

        templateCount = 0;
        templateReady = false;

        corrMean = 0.0;
        corrM2 = 0.0;
        corrCount = 0;

        for (int i = 0; i < winLen; i++)
            sumTemplate[i] = 0.0;
    }

    // -----------------------------
    // build template (beats 2–4)
    // -----------------------------
    if (beatIndex >= 2 && beatIndex <= 4) {

        int start = qrsTimestamp - halfWin - QRS_PIPELINE_DELAY;

        if (start >= 0) {

            for (int i = 0; i < winLen; i++) {
                sumTemplate[i] += signalSample;
            }

            templateCount++;
        }
    }

    // finalize template
    if (beatIndex == 5 && templateCount > 0) {

        for (int i = 0; i < winLen; i++) {
            templateBeat[i] = sumTemplate[i] / templateCount;
        }

        templateReady = true;
    }

    // -----------------------------
    // cross-correlation (beats >= 6)
    // -----------------------------
    if (templateReady && beatIndex >= 6) {

        double num = 0.0;
        double denX = 0.0;
        double denT = 0.0;

        for (int i = 0; i < winLen; i++) {

            double x = beatBuffer[i];
            double t = templateBeat[i];

            num  += x * t;
            denX += x * x;
            denT += t * t;
        }

        double corr = 0.0;

        if (denX > 0.0 && denT > 0.0) {
            corr = num / (sqrt(denX) * sqrt(denT));
        }

        // -----------------------------
        // Welford update
        // -----------------------------
        corrCount++;

        double delta = corr - corrMean;
        corrMean += delta / corrCount;

        double delta2 = corr - corrMean;
        corrM2 += delta * delta2;
    }

    beatIndex++;

    // -----------------------------
    // outputs
    // -----------------------------
    float corrMeanOut = corrMean;

    float corrStdOut = (corrCount > 1)
        ? sqrt(corrM2 / (corrCount - 1))
        : 0.0;
}