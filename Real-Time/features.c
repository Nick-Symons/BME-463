#include <math.h>
#include "features.h"

float calc_rr_std(int* peaks, int count, int fs)
{
    if(count < 4) return 0;

    float rr[32];
    int n = count - 1;
    float mean = 0;

    for(int i=1;i<count;i++)
    {
        rr[i-1]=(peaks[i]-peaks[i-1])*1000.0f/fs;
        mean += rr[i-1];
    }

    mean /= n;

    float var=0;

    for(int i=0;i<n;i++)
    {
        float d=rr[i]-mean;
        var += d*d;
    }

    return sqrtf(var/n);
}

float calc_noise(float* buf, int n)
{
    float mean=0;

    for(int i=0;i<n;i++) mean+=buf[i];
    mean/=n;

    float var=0;

    for(int i=0;i<n;i++)
    {
        float d=buf[i]-mean;
        var+=d*d;
    }

    return sqrtf(var/n);
}