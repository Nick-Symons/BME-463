# Pico Zero-Dynamic-Memory Replacement for Functions.h

```cpp
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void shift_right(float *in, int n);

float filter_IIR(float a,
                 const float *inx, const float *cx, int nx,
                 const float *iny, const float *cy, int ny);

float filter_FIR(float a, const float *in, const float *c, int n);

float* PanT_Thresh(float n, const float *in, float peakt,
                   float peaki, float npki, float spki,
                   float thresholdi1);

/* output count returned by function */
int resampled_signal_static(const float *input_signal, int input_len,
                            float *out, int max_out);

int bandpassed_signal_static(const float *input_signal, int input_len,
                             float *out, int max_out);

int QRS_Peaks_static(const float *input_signal, int input_len,
                     int *out_peaks, int max_peaks);

int extractQRSTimestamps_static(const int *pulse_idx, int pulse_count,
                                int *timestamps, int max_ts,
                                float threshold);

int computeRRIntervals_static(const int *timestamps, int ts_count,
                              int *rrIntervals, int max_rr);

int computeSlidingEntropy_static(const int *rrIntervals, int rr_count,
                                 float fs, int windowSize,
                                 float *out, int max_out);

int computeKurtosis_static(const float *inputSignal, int len,
                           float *out, int max_out);

int computeHF_Residual_static(const float *inputSignal, int len,
                              float *out, int max_out);

int P_wave_BPF_static(const float *signal, int len,
                      int windowSamples,
                      float *out, int max_out);

int P_wave_Amplitude_static(const float *signal, int len,
                            const int *qrs_timestamps, int qrs_count,
                            int searchStart, int searchEnd,
                            float *out, int max_out);

#ifdef __cplusplus
}
#endif

#endif
```

