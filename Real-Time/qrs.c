#include "qrs.h"

#define RR_MAX 32

static int peaks[RR_MAX];
static int count = 0;

static float last3[3] = {0};

void qrs_update(float x, int idx)
{
    last3[0]=last3[1];
    last3[1]=last3[2];
    last3[2]=x;

    if(last3[1]>last3[0] &&
       last3[1]>last3[2] &&
       last3[1]>0.25f)
    {
        if(count==0 || idx-peaks[count-1]>50)
        {
            if(count<RR_MAX)
                peaks[count++]=idx;
        }
    }
}

int qrs_peak_count(void){ return count; }
int* qrs_peaks(void){ return peaks; }