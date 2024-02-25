#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <complex.h>
#include "get_samples.h"
#include "filters.h"

#define DEFAULT_SAMPLE_RATE	1920000
#define NUM_SAMPLES 6291456
#define DECIMATION_FACTOR 8
#define FREQ_DEVIATION 75000


int main(){

    int i,j=0,k;
    uint8_t *samples;
    float  *demodulated_samples = malloc((NUM_SAMPLES/DECIMATION_FACTOR)* sizeof(float));
    double  *deEmphased_samples = malloc((NUM_SAMPLES/DECIMATION_FACTOR)* sizeof(double));
    double *deEmphased_samples2 = malloc((NUM_SAMPLES/(DECIMATION_FACTOR*5))* sizeof(double));
    float    *decimated_samples = malloc(((NUM_SAMPLES/DECIMATION_FACTOR)/5) * sizeof(float));

    FILE* samples_saved = fopen("audio.raw", "wb");

    //initializing dongle parameters
    struct dongle_parameters dp;
    dp.gain = 0;
    dp.ppm_error = 0;
    dp.sync_mode = 0;
    dp.frequency = 91.8e6;
    dp.samp_rate = DEFAULT_SAMPLE_RATE;

    //read samples from dongle
    samples = read_samples(NUM_SAMPLES, &dp);

    fclose(samples_saved);
    free(demodulated_samples);
    free(deEmphased_samples);
    free(deEmphased_samples2);
    free(decimated_samples);
    free(samples);

    printf("All ok!\n");

    return 0;

    float complex *complex_samples = (float complex *) malloc(NUM_SAMPLES * sizeof(float complex));

    //Deleting DC component and building complex samples
    for(i=0, j=0; i< NUM_SAMPLES*2;j++){
        complex_samples[j] = (*(samples + (i)*sizeof(uint8_t)) / (127.5) - 1) + I * (*(samples + (i+1)*sizeof(uint8_t)) / (127.5) - 1);
        i=i+2;
    }

    //filtering selected radio station. Every station signal has 200kHz bandwidth
    float complex *y = (float complex *) malloc(NUM_SAMPLES * sizeof(float complex));
    fir_filter(complex_samples,y,NUM_SAMPLES);

    float complex *decims = (float complex *) malloc(NUM_SAMPLES/DECIMATION_FACTOR * sizeof(float complex));;

    //decimating from 1920000 to 240000 Hz (factor 8)
    for (i = 0, j= 0; i < NUM_SAMPLES;j++ )
    {
        decims[j] = y[i];
        i=i+8;
    }

    free(y);
    free(complex_samples);

    //Demodulating using polar discrimination factor
    for (j = 1; j <= NUM_SAMPLES/DECIMATION_FACTOR; j++)
    {
        demodulated_samples[j-1] = cargf(decims[j] * conj(decims[j-1])) * (((DEFAULT_SAMPLE_RATE)/DECIMATION_FACTOR)/(2*M_PI*FREQ_DEVIATION));
    }

    free(decims);

    /* ----- De-emphasis ----- */

    //using an IIR filter - initializing coefficients
    float d = (DEFAULT_SAMPLE_RATE/DECIMATION_FACTOR) * 50e-6 ; //for Europe 50e-6, for America 75e-6
    float xx = exp(-1/d);
    float b_coeffs[1] = {1-xx};   
    float a_coeffs[2] = {1,-xx};

    float *y1 = (float*) malloc(NUM_SAMPLES/DECIMATION_FACTOR * sizeof(float));

    for ( i = 0; i < NUM_SAMPLES/DECIMATION_FACTOR; i++)
    {
        y1[i] = 0;
    }

    //filtering...
    iir_filter(demodulated_samples,y1,NUM_SAMPLES/DECIMATION_FACTOR,b_coeffs,1,a_coeffs,2);
    
    /* ----------------------- */

    //Decimating to pass to 48kHz so that we can use the audio card
    for (int i = 0, j=0; i < (NUM_SAMPLES/DECIMATION_FACTOR); j++)
    {
        deEmphased_samples2[j] = y1[i];
        i = i+ 5;
    }

    //saving
    for(i=0; i<((NUM_SAMPLES)/(DECIMATION_FACTOR*5));  i++){
        decimated_samples[i] =  (float) deEmphased_samples2[i];
        fwrite(&decimated_samples[i], 4 , 1,  samples_saved);
    }

    fclose(samples_saved);
    free(demodulated_samples);
    free(deEmphased_samples);
    free(deEmphased_samples2);
    free(decimated_samples);
    free(samples);

    return 0;

}