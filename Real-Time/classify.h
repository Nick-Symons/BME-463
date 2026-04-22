#ifndef CLASSIFY_H
#define CLASSIFY_H

typedef enum {
    NORMAL,
    AFIB,
    NOISY,
    OTHER
} RhythmClass;

RhythmClass classify(float noise, float rrstd, int peaks);
const char* class_name(RhythmClass c);

#endif