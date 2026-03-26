/**
 * @file Filters.c
 * @author Zerind Spore
 * @author "YOUR NAME HERE"
 * @brief Digital filter implementations.
 * @version 1.0
 * @date 10/29/2025
 *
 * For lab 4, you will implement function IIR and Shift_Right
 * 
 * For lab 5, you will implement function FIR
 * 
 * 
*/
#include "Filters.h"



/**
 * @brief Shift an array of floats right by one index, DO IN LAB 04
 * 
 * @param in    Pointer to array to be shifted by one
 * @param n     Number of elements in the array
 */ 
void shift_right(float *in, int const n)
{
    // Move elements toward the end of the array
    // Start from the back so we don't overwrite early samples too soon
    // Implement this using a for loop decrementing from n-1 to 1
    
    // Put 0.0 in the newest slot
    for (int i = n - 1; i > 0; i--)
    {
        in[i] = in[i - 1];
    }

    in[0] = 0.0f;
}


/**
 * @brief Implementation of an IIR filter, DO IN LAB 04
 * 
 * @param a     : overall attenuation/gain
 * @param inx   : input signal array pointer, newest sample at index 0
 * @param cx    : numerator coefficients to dot product with inx
 * @param nx    : number of numerator coefficients (cx)
 * @param iny   : input denominator array pointer, newest sample at index 0   
 * @param cy    : denominator coefficients to dot product with iny
 * @param ny    : number of denominator coefficients (cy)
 * 
 * @return y[0]
 */
float filter_IIR(float const a,
                 float const *inx, float const *cx, int const nx,
                 float const *iny, float const *cy, int const ny)
{
    float num = 0.0f;
    float den = 0.0f;
 
        // ---- Numerator sum ----
    for (int i = 0; i < nx; i++)
    {
        num += inx[i] * cx[i];
    }

    // ---- Denominator sum (skip i = 0) ----
    for (int i = 1; i < ny; i++)
    {
        den += iny[i] * cy[i];
    }
    // Output y[0]
    return a * num - den;
}

///////////////////////////////////////////////////////////////////////
///////////// STOP! DO NOT IMPLEMENT FIR UNTIL LAB 05 /////////////////
///////////////////////////////////////////////////////////////////////

/**
 * @brief Implementation of an FIR filter
 *  to mentally track, in[0] is newest sample, in[1] is 1 sample ago, etc.
 * 
 * @param a     : overall attenuation/gain
 * @param in    : input signal array pointer, newest sample at index 0
 * @param c     : FIR coefficients
 * @param n     : number of coefficients
 * 
 * @return newest output sample
 */
float filter_FIR(float const a, float const *in,
                 float const *c,
                 int const n)
{
    float acc = 0.0f;

    // Multiply and accumulate all coefficients:
    for (int i = 0; i < n; i++)
    {
        acc += in[i] * c[i];
    }
  
    // Apply overall gain / attenuation 
    acc = acc * a;

    return acc;
}

float filter_FIR_i(float const a, float const *in,
                 int const *c,
                 int const n)
{
    float acc = 0.0f;

    // Multiply and accumulate all coefficients:
    for (int i = 0; i < n; i++)
    {
        acc += in[i] * c[i];
    }
  
    // Apply overall gain / attenuation 
    acc = acc * a;

    return acc;
}




///////////////////////////////////////////////////////////////////////
///// STOP! DO NOT IMPLEMENT PAN TOMPKINS THRESHOLD UNTIL LAB 10 //////
///////////////////////////////////////////////////////////////////////

float* PanT_Thresh(float const n, float const *in, float peakt, float peaki, float npki, float spki, float thresholdi1)
{
    // To be implemented in Lab 10
    return 0;
}
