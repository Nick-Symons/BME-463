#ifndef QRS_H
#define QRS_H

void qrs_update(float sample, int index);
int qrs_peak_count(void);
int* qrs_peaks(void);

#endif