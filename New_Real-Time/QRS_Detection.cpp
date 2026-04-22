# Pico Zero-Dynamic-Memory Conversion of QRS_Detection.cpp

```cpp
#include <math.h>
#include <string.h>
#include "functions.h"

#define ORDER 48
#define LOW_PASS_N 11
#define HIGH_PASS_N 32
#define DERIVATIVE_N 5
#define MWI_N 32
#define MAX_INIT 400

static float x_state[ORDER]={0};
static float x_low_pass[LOW_PASS_N]={0};
static float x_high_pass[HIGH_PASS_N]={0};
static float x_derivative[DERIVATIVE_N]={0};
static float x_mwi[MWI_N]={0};

static const float cx[ORDER]={-0.001621,-0.001222,0.002827,0.010480,0.016633,
0.014543,0.002976,-0.009870,-0.012021,0.000086,
0.015180,0.016004,-0.002277,-0.023029,-0.021805,
0.006816,0.036836,0.031457,-0.017088,-0.067212,
-0.054252,0.052347,0.210995,0.328564,0.328564,
0.210995,0.052347,-0.054252,-0.067212,-0.017088,
0.031457,0.036836,0.006816,-0.021805,-0.023029,
-0.002277,0.016004,0.015180,0.000086,-0.012021,
-0.009870,0.002976,0.014543,0.016633,0.010480,
0.002827,-0.001222,-0.001621};

static const float c_low_pass[LOW_PASS_N]={1,2,3,4,5,6,5,4,3,2,1};
static const float c_high_pass[HIGH_PASS_N]={-.03125,-.03125,-.03125,-.03125,-.03125,
-.03125,-.03125,-.03125,-.03125,-.03125,
-.03125,-.03125,-.03125,-.03125,-.03125,
0.96875,-.03125,-.03125,-.03125,-.03125,
-.03125,-.03125,-.03125,-.03125,-.03125,
-.03125,-.03125,-.03125,-.03125,-.03125,
-.03125,-.03125};

static const float c_derivative[DERIVATIVE_N]={2,1,0,-1,-2};

static const float c_mwi[MWI_N]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

int resampled_signal_static(const float *in,int len,float *out,int max_out){
 int count=0;
 for(int i=0;i<len;i++){
   for(int u=0;u<2;u++){
     float s=(u==0)?in[i]:0.0f;
     shift_right(x_state,ORDER); x_state[0]=s;
     float y=filter_FIR(2.0f,x_state,cx,ORDER);
     static int dec=0;
     if(dec==0 && count<max_out) out[count++]=y;
     dec=(dec+1)%3;
   }
 }
 return count;
}

int bandpassed_signal_static(const float *in,int len,float *out,int max_out){
 int n=0;
 for(int i=0;i<len && n<max_out;i++){
   shift_right(x_low_pass,LOW_PASS_N); x_low_pass[0]=in[i];
   float y1=filter_FIR(1.0f/36.0f,x_low_pass,c_low_pass,LOW_PASS_N);
   shift_right(x_high_pass,HIGH_PASS_N); x_high_pass[0]=y1;
   out[n++]=filter_FIR(1.0f/1.216f,x_high_pass,c_high_pass,HIGH_PASS_N);
 }
 return n;
}

int QRS_Peaks_static(const float *in,int len,int *peaks,int max_peaks){
 float mwi[MAX_INIT>4096?4096:MAX_INIT];
 int mlen = (len<400)?len:400;
 memset(x_derivative,0,sizeof(x_derivative));
 memset(x_mwi,0,sizeof(x_mwi));
 for(int i=0;i<mlen;i++){
   shift_right(x_derivative,DERIVATIVE_N); x_derivative[0]=in[i];
   float y=filter_FIR(1.0f/8.0f,x_derivative,c_derivative,DERIVATIVE_N);
   float sq=y*y*500.0f;
   shift_right(x_mwi,MWI_N); x_mwi[0]=sq;
   mwi[i]=filter_FIR(1.0f,x_mwi,c_mwi,MWI_N);
 }
 float maxInit=0.0f; for(int i=0;i<mlen;i++) if(mwi[i]>maxInit) maxInit=mwi[i];
 float SPKI=0.25f*maxInit, NPKI=0.125f*maxInit;
 float TH1=NPKI+0.25f*(SPKI-NPKI);
 memset(x_derivative,0,sizeof(x_derivative)); memset(x_mwi,0,sizeof(x_mwi));
 float localMax=0; int localIdx=-1; int inPeak=0; int since=1000; int pc=0;
 for(int i=0;i<len;i++){
   shift_right(x_derivative,DERIVATIVE_N); x_derivative[0]=in[i];
   float y=filter_FIR(1.0f/8.0f,x_derivative,c_derivative,DERIVATIVE_N);
   float sq=y*y*500.0f;
   shift_right(x_mwi,MWI_N); x_mwi[0]=sq;
   float m=filter_FIR(1.0f,x_mwi,c_mwi,MWI_N);
   since++;
   if(m>localMax){localMax=m; localIdx=i; inPeak=1;}
   if(inPeak && m<localMax*0.9f){
      inPeak=0;
      if(localMax>=TH1 && since>40){ if(pc<max_peaks) peaks[pc++]=localIdx; since=0; SPKI=0.125f*localMax+0.875f*SPKI; }
      else NPKI=0.125f*localMax+0.875f*NPKI;
      TH1=NPKI+0.25f*(SPKI-NPKI); localMax=0;
   }
 }
 return pc;
}

int computeRRIntervals_static(const int *ts,int n,int *rr,int max_rr){int c=0; for(int i=1;i<n&&c<max_rr;i++) rr[c++]=ts[i]-ts[i-1]; return c;}

int computeSlidingEntropy_static(const int *rr,int n,float fs,int ws,float *out,int max_out){
 const float bins[11]={0,0.3f,0.5f,0.6f,0.7f,0.8f,0.9f,1.0f,1.2f,1.5f,99};
 int c=0;
 for(int i=ws;i<=n && c<max_out;i++){
   int hist[10]={0};
   for(int j=i-ws;j<i;j++){
     float r=rr[j]/fs;
     for(int b=0;b<10;b++) if(r>=bins[b] && r<bins[b+1]){hist[b]++; break;}
   }
   float e=0;
   for(int b=0;b<10;b++) if(hist[b]){ float p=(float)hist[b]/ws; e -= p*(logf(p)/logf(2.0f)); }
   out[c++]=e;
 }
 return c;
}
```
