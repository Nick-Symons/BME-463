#include "filters.h"

static float hp_prev = 0;
static float x_prev = 0;
static float lp_prev = 0;

static float highpass(float x)
{
    float y = 0.995f * (hp_prev + x - x_prev);
    hp_prev = y;
    x_prev = x;
    return y;
}

static float lowpass(float x)
{
    lp_prev += 0.1f * (x - lp_prev);
    return lp_prev;
}

float ecg_filter(float x)
{
    return lowpass(highpass(x));
}