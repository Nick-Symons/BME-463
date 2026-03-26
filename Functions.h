
#include <stdint.h>

/**
 * @brief Shift an array of floats right by one index
 * 
 * @param in    Pointer to array to shift
 * @param n     Length of array
 */ 
void shift_right(float *in, int const n); // Implemented during lab 04


/**
 * @brief Implementation of an IIR filter
 * 
 * @param a     : overall attenuation/gain
 * @param inx   : input signal array pointer, newest sample at index 0
 * @param cx    : numerator coefficients
 * @param nx    : number of numerator coefficients
 * @param iny   : output signal array pointer, newest output at index 0   
 * @param cy    : denominator coefficients
 * @param ny    : number of denominator coefficients
 * 
 * @return y[0]
 */
float filter_IIR(float const a,
                 float const *inx, float const *cx, int const nx,
                 float const *iny, float const *cy, int const ny);

/**
 * @brief Implementation of an FIR filter
 * 
 * @param a     : overall attenuation/gain
 * @param in    : input signal array pointer, newest sample at index 0
 * @param c     : FIR coefficients
 * @param n     : number of coefficients
 * 
 * @return newest output sample
 */
float filter_FIR(float const a,
                 float const *in,
                 float const *c,
                 int const n);
                 // Implemented during lab 05



///////////////////////////////////////////////////////////////////////
///// STOP! DO NOT IMPLEMENT PAN TOMPKINS THRESHOLD UNTIL LAB 10 //////
///////////////////////////////////////////////////////////////////////

float* PanT_Thresh(float const n, float const *in, float peakt, float peaki, float npki, float spki, float thresholdi1);
