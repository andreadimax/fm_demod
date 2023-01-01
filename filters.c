#include "filters.h"


#define ORDER 39

float fir_coeffs[ORDER] =  {
    -0.000175388381457631,
    -0.001081578864783447,
    -0.001952816940761620,
    -0.002339542890303031,
    -0.001441043405376425,
    0.001347804896207034,
    0.005616827900957026,
    0.009446763814526433,
    0.009829843166274001,
    0.004146468717566274,
    -0.007842989084981804,
    -0.022556389911108012,
    -0.032759965041076984,
    -0.029771738254108050,
    -0.007094214896639929,
    0.036021911732894665,
    0.092830235703578734,
    0.150286158813892540,
    0.193054813051436985,
    0.208869679746526316,
    0.193054813051437013,
    0.150286158813892540,
    0.092830235703578762,
    0.036021911732894679,
    -0.007094214896639929,
    -0.029771738254108050,
    -0.032759965041076977,
    -0.022556389911108015,
    -0.007842989084981808,
    0.004146468717566274,
    0.009829843166274006,
    0.009446763814526441,
    0.005616827900957029,
    0.001347804896207034,
    -0.001441043405376424,
    -0.002339542890303034,
    -0.001952816940761620,
    -0.001081578864783446,
    -0.000175388381457631,
};

/**
 * @brief A generic FIR filter
 * 
 * @param input_complex Input vector with samples to save
 * @param output_complex Output vector where filtered samples will be saved
 * @param length Length of input_complex and output complex
 */
void fir_filter(float complex* input_complex, float complex* output_complex, int length)
{

    double acc;     // accumulator for sum
    int n = 0;
    int k = 0;
    int i = 0;

    /**
     * The input signal is complex and we can treat it as
     * two separate signals (real and image parts). Filter 
     * is applied to both signals 
     */
    float* input_real = (float*) malloc(length * sizeof(float));
    float* input_imag = (float*) malloc(length * sizeof(float));
    float* output_real = (float*) malloc(length * sizeof(float));
    float* output_imag = (float*) malloc(length * sizeof(float));

    
    for (i = 0; i < length; i++)
    {
        input_real[i] = crealf(input_complex[i]);
        input_imag[i] = cimagf(input_complex[i]);
    }
 
    // apply the filter to each input sample of each signal
    for (n = 0; n < length; n++) {
        acc = 0;
        for ( k = 0; k < ORDER; k++ ) {
            if(n-k >= 0 && n-k <=(length-1)){
                acc += fir_coeffs[k] * input_real[n-k];
            }
        }
        output_real[n] = acc;
    }

    for (n = 0; n < length; n++) {
        acc = 0;
        for ( k = 0; k < ORDER; k++ ) {
            if(n-k >= 0 && n-k <=(length-1)){
                acc += fir_coeffs[k] * input_imag[n-k];
            }
        }
        output_imag[n] = acc;
    }

    //rebuilding a single (filtered) signal
    for (i = 0; i < length; i++)
    {
        output_complex[i] = output_real[i] + I * output_imag[i];
    }

    free(input_imag);
    free(input_real);
    free(output_real);
    free(output_imag);

    return;
 
}

/**
 * @brief A generic IIR filter
 * 
 * @param input  Input samples vector
 * @param output Vector where filtered samples will be saved
 * @param length Length of input and output vectors
 * @param b      Vector with numerator coefficients
 * @param l_b    Length of b
 * @param a      Vector with denumerator coefficients
 * @param l_a   Length of a
 */
void iir_filter(float* input,float* output,int length, float*b,int l_b, float* a, int l_a){

    double acc;     // accumulator for sum
    int n = 0;
    int k = 0;
    int i = 0;

    for (n = 0; n < length; n++) {
        acc = 0;
        for ( k = 0; k < l_b; k++ ) {
            if(n-k >= 0 && n-k <=(length-1)){
                acc += b[k] * input[n-k];
            }
        }

        for ( k = 1; k < l_a; k++ ) {
            if(n-k >= 0 && n-k <=(length-1)){
                acc -= a[k] * output[n-k];
            }
        }
        output[n] = acc / a[0];
    }

    return;


}