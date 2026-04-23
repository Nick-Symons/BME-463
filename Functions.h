
#include <stdint.h>
#include <vector>
#include <fstream>

using namespace std;

void shift_right(float *in, int const n); 

float filter_IIR(float const a,
                 float const *inx, float const *cx, int const nx,
                 float const *iny, float const *cy, int const ny);

float filter_FIR(float const a, float const *in, float const *c,
                 int const n);

float* PanT_Thresh(float const n, float const *in, float peakt, 
                    float peaki, float npki, float spki, 
                    float thresholdi1);

vector<double> resampled_signal(const vector<double>& input_signal);

vector<double> bandpassed_signal(const vector<double>& input_signal);

vector<double> QRS_Peaks(const vector<double>& input_signal);

vector<int> extractQRSTimestamps(const vector<double>& pulseVec, 
                    double threshold = 0.5);

vector<int> computeRRIntervals(const vector<int>& timestamps);

vector<double> computeSlidingEntropy(const vector<int>& rrIntervals, 
                    double fs = 200.0, int windowSize = 15);

vector<double> computeKurtosis(const vector<double>& inputSignal);

vector<double> computeHF_Residual(const vector<double>& inputSignal);

vector<double> P_wave_BPF(const vector<double>& signal, 
                    int windowSamples = 600);

vector <double> P_wave_Amplitude(const vector<double>& signal, const vector<int>& qrs_timestamps, 
                                                        int searchStart = -35, int searchEnd = -12);

double computeTemplateDifference(const vector<double>& signal, const vector<int>& timestamps, vector<double>& templateOut, vector<double>& diffSignal, int halfWin = 60);

double computeRMSSD(const vector<int>& rrIntervals, double fs = 200.0);