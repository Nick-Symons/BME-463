#include <stdint.h>

#define BEAT_WINDOW 14

void shift_right(float *in, int n);

float filter_IIR(float a,
                 const float *inx, const float *cx, int nx,
                 const float *iny, const float *cy, int ny);

float filter_FIR(float a, const float *in, const float *c, int n);

void QRS_Detection_run(float new_input_sample);

float computeSlidingEntropy(const float rr_Intervals[BEAT_WINDOW]);

double computeKurtosis(double new_input_sample);

void computeTemplateCrossCorrelation(double signalSample,
                                     int qrsTimestamp,
                                     int halfWin);