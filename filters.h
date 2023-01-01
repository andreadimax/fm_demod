#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

void fir_filter( float complex* input_complex, float complex* output_complex, int length);
void iir_filter(float* input,float* output,int length, float*b,int l_b, float* a, int l_a);