# Pico Zero-Dynamic-Memory Conversion of PWaveScore.cpp

```cpp
#include <math.h>
#include <string.h>
#include "functions.h"

static float x_high_pass_pw[2]={0};
static float y_high_pass_pw[2]={0};
static const float cx_high_pass_pw[2]={1.0f,-1.0f};
static const float cy_high_pass_pw[2]={1.0f,-0.9793759f};
static const float a_high_pass_pw=0.98968796f;

#define PW_LOW_PASS_N 51
static float x_low_pass_pw[PW_LOW_PASS_N]={0};
static const float c_low_pass_pw[PW_LOW_PASS_N]={0.0015218984,0.0022943561,0.0035908644,0.0048960707,0.0058952848,0.0062088716,0.0054541546,0.0033346423,-0.00026836312,-0.0052135936,-0.011046785,-0.01699728,-0.0220346,-0.02498355,-0.024687104,-0.020196246,-0.010956039,0.0030464116,0.021201625,0.04228503,0.0645598,0.08596756,0.10438147,0.11788369,0.12502475,0.12502475,0.11788369,0.10438147,0.08596756,0.0645598,0.04228503,0.021201625,0.0030464116,-0.010956039,-0.020196246,-0.024687104,-0.02498355,-0.0220346,-0.01699728,-0.011046785,-0.0052135936,-0.00026836312,0.0033346423,0.0054541546,0.0062088716,0.0058952848,0.0048960707,0.0035908644,0.0022943561,0.0015218984};

int P_wave_BPF_static(const float *signal,int len,int windowSamples,float *out,int max_out){
 (void)windowSamples;
 static float hpbuf[4096];
 if(len>4096) len=4096;
 memset(x_high_pass_pw,0,sizeof(x_high_pass_pw));
 memset(y_high_pass_pw,0,sizeof(y_high_pass_pw));
 memset(x_low_pass_pw,0,sizeof(x_low_pass_pw));
 for(int i=0;i<len;i++){
   shift_right(x_high_pass_pw,2);
   shift_right(y_high_pass_pw,2);
   x_high_pass_pw[0]=signal[i];
   float y=filter_IIR(a_high_pass_pw,x_high_pass_pw,cx_high_pass_pw,2,y_high_pass_pw,cy_high_pass_pw,2);
   y_high_pass_pw[0]=y;
   hpbuf[i]=y;
 }
 int n=0;
 for(int i=0;i<len && n<max_out;i++){
   shift_right(x_low_pass_pw,PW_LOW_PASS_N);
   x_low_pass_pw[0]=hpbuf[i];
   out[n++]=filter_FIR(1.0f,x_low_pass_pw,c_low_pass_pw,PW_LOW_PASS_N);
 }
 return n;
}

int P_wave_Amplitude_static(const float *signal,int len,const int *qrs,int qrs_count,int searchStart,int searchEnd,float *out,int max_out){
 int c=0;
 for(int k=0;k<qrs_count && c<max_out;k++){
   int ts=qrs[k];
   int start=ts+searchStart; if(start<0) start=0;
   int end=ts+searchEnd; if(end>=len) end=len-1;
   if(end<start){ out[c++]=0.0f; continue; }
   float m=signal[start];
   for(int i=start+1;i<=end;i++) if(signal[i]>m) m=signal[i];
   out[c++]=m;
 }
 return c;
}
```
