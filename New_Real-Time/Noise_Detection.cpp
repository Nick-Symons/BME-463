# Pico Zero-Dynamic-Memory Conversion of Noise_Detection.cpp

```cpp
#include <math.h>
#include <string.h>
#include "functions.h"

#define NOISE_WINDOW 400
#define HF_N 41

static float x_hf[HF_N]={0};
static const float c_hf[HF_N]={
0.0138121f,-0.00978287f,-0.00948737f,-0.00910602f,
-0.00700129f,-0.00225793f,0.00500932f,0.0135511f,
0.0211221f,0.0249158f,0.0221847f,0.0110238f,
-0.00912934f,-0.0369226f,-0.0693216f,-0.101989f,
-0.130009f,-0.148919f,0.844406f,-0.148919f,
-0.130009f,-0.101989f,-0.0693216f,-0.0369226f,
-0.00912934f,0.0110238f,0.0221847f,0.0249158f,
0.0211221f,0.0135511f,0.00500932f,-0.00225793f,
-0.00700129f,-0.00910602f,-0.00948737f,-0.00978287f,
0.0138121f};

int computeKurtosis_static(const float *in,int len,float *out,int max_out){
 int c=0;
 for(int base=0;base+NOISE_WINDOW<=len && c<max_out;base+=NOISE_WINDOW){
   float mean=0.0f;
   for(int i=0;i<NOISE_WINDOW;i++) mean += in[base+i];
   mean /= (float)NOISE_WINDOW;
   float var=0.0f, kurt=0.0f;
   for(int i=0;i<NOISE_WINDOW;i++){
     float d=in[base+i]-mean;
     float d2=d*d;
     var += d2;
     kurt += d2*d2;
   }
   var /= (float)NOISE_WINDOW;
   kurt /= (float)NOISE_WINDOW;
   out[c++] = (var>0.0f)?(kurt/(var*var)):0.0f;
 }
 return c;
}

int computeHF_Residual_static(const float *in,int len,float *out,int max_out){
 memset(x_hf,0,sizeof(x_hf));
 int c=0;
 for(int i=0;i<len && c<max_out;i++){
   shift_right(x_hf,HF_N);
   x_hf[0]=in[i];
   float y=filter_FIR(1.0f,x_hf,c_hf,HF_N);
   out[c++]=y*y;
 }
 return c;
}
```
