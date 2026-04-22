#include "classify.h"

RhythmClass classify(float noise, float rrstd, int peaks)
{
    if(noise < 0.01f || noise > 1.5f)
        return NOISY;

    if(rrstd > 120.0f)
        return AFIB;

    if(rrstd < 60.0f && peaks > 8)
        return NORMAL;

    return OTHER;
}

const char* class_name(RhythmClass c)
{
    switch(c)
    {
        case NORMAL: return "NORMAL";
        case AFIB: return "AFIB";
        case NOISY: return "NOISY";
        default: return "OTHER";
    }
}