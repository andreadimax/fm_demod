#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <complex.h>
#include <liquid/liquid.h>
#include "get_samples.h"
#include "filters.h"

#define DEFAULT_SAMPLE_RATE	1920000
#define TIME_CONSTANT 0.000050
#define FREQ_DEVIATION  75000
#define NUM_SAMPLES 6291456
#define BUF_LENGHT  4096
#define DECIMATION_FACTOR 8
#define TAU 0.000075

#define COUNT       NUM_SAMPLES/BUF_LENGHT

int filedes[2];


void RC_filter(double* input, double* output, int points, double alpha) 
{
    output[0] = input[0];
    for(int i = 1; i < points; ++i) 
    {  
        output[i] = output[i-1] + (alpha*(input[i] - output[i-1])); 
        //printf("%f\n", output[i]);
    } 

    // output[0] = input[0];
    // for(int i = 1; i < points; ++i) 
    // {  
    //     output[i] = input[i] - TAU * output[i-1];
    //     //printf("%f\n", output[i]);
    // } 
} 


int main(){
    int i,j=0,k;
    uint8_t *samples;
    double complex *IQ_BUF = malloc((NUM_SAMPLES/DECIMATION_FACTOR) * sizeof(double complex));
    double *filtered = malloc(2*NUM_SAMPLES*sizeof(double));
    double *demodulated_samples = malloc((NUM_SAMPLES/DECIMATION_FACTOR)* sizeof(double));
    double *deEmphased_samples = malloc((NUM_SAMPLES/DECIMATION_FACTOR)* sizeof(double));
    double *deEmphased_samples2 = malloc((NUM_SAMPLES/(DECIMATION_FACTOR*5))* sizeof(double));
    double *deEmphased_samples3 = malloc((NUM_SAMPLES/(DECIMATION_FACTOR*5))* sizeof(double));
    float *decimated_samples = malloc(((NUM_SAMPLES/DECIMATION_FACTOR)/5) * sizeof(float));
    double *d_samples = malloc(2*NUM_SAMPLES*sizeof(double));
    FILE* fout1 = fopen("IQdata.dat", "wb");
    FILE* fout2 = fopen("audio.raw", "wb");

    double dt = (-1)/(240e3 *  50e-6); 
    double alpha = 1-exp(dt);

    struct dongle_parameters dp;
    dp.gain = 0;
    dp.ppm_error = 0;
    dp.sync_mode = 0;
    dp.frequency = 100.4e6;
    dp.samp_rate = DEFAULT_SAMPLE_RATE;
    // /*
    //     We want to hear 10 seconds -> 19200000 samples
    //     Nearest 16384 multiple is 19202048
    // */
    // dp.out_block_size = 19202048;


    samples = read_samples(NUM_SAMPLES, &dp);

    float complex *complex_samples = (float complex *) malloc(NUM_SAMPLES * sizeof(float complex));

    //Deleting DC component
    for(i=0, j=0; i< NUM_SAMPLES*2;j++){
        //fwrite(samples + i*sizeof(uint8_t),1,1,fout1);
        // double temp = (double)(*(samples + i*sizeof(uint8_t)) / (127.5) - 1);
        // d_samples[i] = temp;
        complex_samples[j] = (*(samples + (i)*sizeof(uint8_t)) / (127.5) - 1) + I * (*(samples + (i+1)*sizeof(uint8_t)) / (127.5) - 1);
        i=i+2;
    }

    //fclose(fout1);

    //Filtering
    //low_pass(d_samples, filtered, NUM_SAMPLES*2);

    /* --- Liquid DSP FIltering --- */

    // ... initialize filter coefficients ...
    float h[64] = { 0.00079199,  0.00076934,  0.00042484, -0.0002186,  -0.00100941, -0.00162849,
        -0.00165806, -0.00078267 , 0.00095162 , 0.00297498 , 0.00428583  ,0.0038498,
        0.00118316, -0.00315776 ,-0.00749774 ,-0.00955615 ,-0.00745876 ,-0.00085647,
        0.00840521,  0.01653544  ,0.01909092  ,0.01295404 ,-0.00176983 ,-0.02082547,
        -0.03647356, -0.03977745 ,-0.02384313  ,0.01315647  ,0.06640549 , 0.12510427,
        0.1753408,   0.20428935  ,0.20428935  ,0.1753408   ,0.12510427  ,0.06640549,
        0.01315647, -0.02384313 ,-0.03977745 ,-0.03647356 ,-0.02082547 ,-0.00176983,
        0.01295404,  0.01909092  ,0.01653544  ,0.00840521 ,-0.00085647 ,-0.00745876,
        -0.00955615, -0.00749774 ,-0.00315776  ,0.00118316 , 0.0038498  , 0.00428583,
        0.00297498,  0.00095162 ,-0.00078267 ,-0.00165806 ,-0.00162849 ,-0.00100941,
        -0.0002186 ,  0.00042484 , 0.00076934 , 0.00079199};         // filter coefficients


    // create filter object
    firfilt_crcf q = firfilt_crcf_create(h,64);

    float complex *y = (float complex *) malloc(NUM_SAMPLES * sizeof(float complex));    // output sample

    // execute filter (repeat as necessary)
    for (i = 0; i < NUM_SAMPLES; i++)
    {
        firfilt_crcf_push(q, complex_samples[i]);    // push input sample
        firfilt_crcf_execute(q,&y[i]); // compute output
    }

    // destroy filter object
    firfilt_crcf_destroy(q);

    /* ---------------------------- */

    float complex *decims = (float complex *) malloc(NUM_SAMPLES/DECIMATION_FACTOR * sizeof(float complex));;

    for (i = 0, j= 0; i < NUM_SAMPLES;j++ )
    {
        decims[j] = y[i];
        i=i+8;
    }

    for (j = 1; j <= NUM_SAMPLES/DECIMATION_FACTOR; j++)
    {
        demodulated_samples[j-1] = carg(decims[j] * conj(decims[j-1])) * (((DEFAULT_SAMPLE_RATE)/DECIMATION_FACTOR)/(2*M_PI*FREQ_DEVIATION));
        fwrite(&demodulated_samples[j-1],8,1,fout1);
    }

    fclose(fout1);

    //return 0;   
    
    

    // for(i=0,j=0;i<(NUM_SAMPLES*2);j++){

    //     IQ_BUF[j] = filtered[i] + filtered[i+1] * I;
        
    //     i= i+16;    //decimating...

    //     if(j>=1){
    //         demodulated_samples[j-1] = carg(IQ_BUF[j] * conj(IQ_BUF[j-1])) * (((DEFAULT_SAMPLE_RATE)/DECIMATION_FACTOR)/(2*M_PI*FREQ_DEVIATION));
    //     }

    // }

    // demodulated_samples[j-1] = carg(IQ_BUF[j] * conj(IQ_BUF[j-1])) * (((DEFAULT_SAMPLE_RATE)/DECIMATION_FACTOR)/(2*M_PI*FREQ_DEVIATION)) ;

    /* ----- De-emphasis ----- */

    float d = (DEFAULT_SAMPLE_RATE/DECIMATION_FACTOR) * 75e-6 ;// # Calculate the # of samples to hit the -3dB point  
    float xx = exp(-1/d);//   # Calculate the decay between each sample  
    float b_coeffs[1] = {1-xx};   //       # Create the filter coefficients  
    float a_coeffs[2] = {1,-xx};

    // create filter object
    iirfilt_crcf q1 = iirfilt_crcf_create(b_coeffs,1,a_coeffs,2);

    float complex *y1 = (float complex *) malloc(NUM_SAMPLES * sizeof(float complex));    // output sample

    // execute filter (repeat as necessary)
    for (i = 0; i < NUM_SAMPLES/DECIMATION_FACTOR; i++)
    {
        //iirfilt_crcf_push(q1, demodulated_samples[i]);    // push input sample
        iirfilt_crcf_execute(q1,demodulated_samples[i],&y1[i]); // compute output
    }

    // destroy filter object
    iirfilt_crcf_destroy(q1);



    /* ----------------------- */

    //RC_filter(demodulated_samples, deEmphased_samples, ((NUM_SAMPLES)/DECIMATION_FACTOR), alpha);

    for (int i = 0, j=0; i < (NUM_SAMPLES/DECIMATION_FACTOR); j++)
    {
        deEmphased_samples2[j] = y1[i];
        i = i+ 5;
    }
    
    //low_pass2(deEmphased_samples2, deEmphased_samples3, ((NUM_SAMPLES)/(DECIMATION_FACTOR*5)));


    for(i=0; i<((NUM_SAMPLES)/(DECIMATION_FACTOR*5));  i++){
        decimated_samples[i] =  (float) deEmphased_samples2[i];
        fwrite(&decimated_samples[i], 4 , 1,  fout2);
    }

    fclose(fout2);
    //fclose(fout1);
    free(IQ_BUF);
    free(filtered);
    free(demodulated_samples);
    free(deEmphased_samples);
    free(deEmphased_samples2);
    free(deEmphased_samples3);
    free(decimated_samples);
    free(d_samples);
    free(samples);
    return 0;

}