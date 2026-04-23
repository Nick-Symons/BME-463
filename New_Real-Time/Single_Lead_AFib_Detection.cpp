# Pico 2 Real-Time Main Conversion (Zero Dynamic Memory)

```cpp
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "functions.h"

#define FS 300
#define WINDOW 3000
#define MAX_BUF 3000
#define MAX_BEATS 64
#define MAX_RR 64
#define MAX_FEAT 128
#define ADC_PIN 26

static float ecg[WINDOW];
static int wr=0;
static int full=0;

static float work300[WINDOW];
static float resamp[MAX_BUF];
static float band[MAX_BUF];
static int peaks[MAX_BEATS];
static int rr[MAX_RR];
static float entropy[MAX_FEAT];
static float kurt[MAX_FEAT];
static float resid[MAX_BUF];
static float pwave[MAX_BUF];
static int qrs_pw[MAX_BEATS];
static float pamp[MAX_BEATS];

static float meanf(const float *x,int n){float s=0; for(int i=0;i<n;i++) s+=x[i]; return n? s/n:0;}
static float maxf(const float *x,int n){float m=x[0]; for(int i=1;i<n;i++) if(x[i]>m) m=x[i]; return m;}
static float minf(const float *x,int n){float m=x[0]; for(int i=1;i<n;i++) if(x[i]<m) m=x[i]; return m;}

static void linearize(){
 for(int i=0;i<WINDOW;i++) work300[i]=ecg[(wr+i)%WINDOW];
}

void detect_afib_realtime(){
 linearize();

 int nk = computeKurtosis_static(work300,WINDOW,kurt,MAX_FEAT);
 int nr = computeHF_Residual_static(work300,WINDOW,resid,MAX_BUF);
 float meanKurt = meanf(kurt,nk);
 float meanResid = meanf(resid,nr);

 int nRes = resampled_signal_static(work300,WINDOW,resamp,MAX_BUF);
 int nBand = bandpassed_signal_static(resamp,nRes,band,MAX_BUF);
 int nBeats = QRS_Peaks_static(band,nBand,peaks,MAX_BEATS);
 if(nBeats < 2){ printf("NA\n"); return; }

 float recSec = WINDOW / 300.0f;
 float bpm = (nBeats / recSec) * 60.0f;
 if(bpm < 20 || bpm > 220){ printf("NA\n"); return; }

 int nRR = computeRRIntervals_static(peaks,nBeats,rr,MAX_RR);
 if(nRR < 8){ printf("NA\n"); return; }

 int nEnt = computeSlidingEntropy_static(rr,nRR,200.0f,15,entropy,MAX_FEAT);
 if(nEnt <= 0){ printf("NA\n"); return; }

 int nPW = P_wave_BPF_static(resamp,nRes,600,pwave,MAX_BUF);
 (void)nPW;

 for(int i=0;i<nBeats;i++) qrs_pw[i] = peaks[i] - 22;
 int nAmp = P_wave_Amplitude_static(pwave,nRes,qrs_pw,nBeats,-35,-11,pamp,MAX_BEATS);

 float meanEnt = meanf(entropy,nEnt);
 float maxEnt = maxf(entropy,nEnt);
 float minEnt = minf(entropy,nEnt);
 float meanPAmp = meanf(pamp,nAmp);

 printf("Kurt=%.3f Resid=%.3f Ent=%.3f MaxE=%.3f MinE=%.3f P=%.3f BPM=%.1f\n",
 meanKurt,meanResid,meanEnt,maxEnt,minEnt,meanPAmp,bpm);
}

int main(){
 stdio_init_all();
 adc_init();
 adc_gpio_init(ADC_PIN);
 adc_select_input(0);
 sleep_ms(2000);
 printf("AFib Detector Started\n");
 absolute_time_t next=get_absolute_time();
 while(1){
   uint16_t raw=adc_read();
   ecg[wr]=((float)raw/4096.0f)-0.5f;
   wr=(wr+1)%WINDOW;
   if(wr==0) full=1;
   if(full) detect_afib_realtime();
   next=delayed_by_us(next,1000000/FS);
   sleep_until(next);
 }
}
```
