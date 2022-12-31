#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <complex.h>
#include "libsamples.h"
#include "low_pass.h"
#include "fft-complex.h"

#define DEFAULT_SAMPLE_RATE	1920000
#define TIME_CONSTANT 0.000050
#define FREQ_DEVIATION  75000
#define PI 3.141592653589793
#define NUM_SAMPLES 1920000
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

void IIRFloat(double *coeffs_B, double *coeffs_A, double *input, double *output, int length, int filterLength)
{
    double bcc, acc;
    double *inputp;
    int n,k;


    for (int ii=0; ii<filterLength; ii++)
    {
        output[ii] = 0;
    }

    //filter length =7
    for (n = 0; n < length; n++) {
        inputp = &input[filterLength - 1 + n]; //insamp[6]~insamp[85]

        acc = 0;
        bcc = 0;

        for (k = 0; k < filterLength; k++)
        {
            output[n] += (coeffs_B[k] * inputp[filterLength - k - 1] - coeffs_A[k] * output[filterLength - k - 1]);
        }
    }
}


int main(){
    int i,j=0,k;
    uint8_t *samples;
    int cnt = 0;
    double complex *IQ_BUF = malloc((NUM_SAMPLES/DECIMATION_FACTOR) * sizeof(double complex));
    double *average = calloc((BUF_LENGHT), sizeof(double));
    double *filtered = malloc(2*NUM_SAMPLES*sizeof(double));
    double *demodulated_samples = malloc((NUM_SAMPLES/DECIMATION_FACTOR)* sizeof(double));
    double *deEmphased_samples = malloc((NUM_SAMPLES/DECIMATION_FACTOR)* sizeof(double));
    double *deEmphased_samples2 = malloc((NUM_SAMPLES/(DECIMATION_FACTOR*5))* sizeof(double));
    double *deEmphased_samples3 = malloc((NUM_SAMPLES/(DECIMATION_FACTOR*5))* sizeof(double));
    float *decimated_samples = malloc(((NUM_SAMPLES/DECIMATION_FACTOR)/5) * sizeof(float));
    double *d_samples = malloc(2*NUM_SAMPLES*sizeof(double));
    FILE* fout1 = fopen("IQdata.dat", "w");
    FILE* fout2 = fopen("audio.raw", "wb");
    FILE* first_filtered = fopen("first_filtered.txt", "w");
    FILE* demodulated = fopen("demodulated.txt", "w");
    FILE* out2 = fopen("out2.txt", "w");
    FILE* rf = fopen("rf.dat", "wb");
    // SF_INFO sf_info;

    // sf_info.channels=1;
    // sf_info.samplerate = 48000;
    // sf_info.frames = (sf_count_t) ((NUM_SAMPLES/DECIMATION_FACTOR)/5);
    // sf_info.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT);
    // sf_info.sections = 0;
    // sf_info.seekable = 0;

    // SNDFILE* file_snd = sf_open("audio.wav", SFM_WRITE, &sf_info);

    double w_c = 1 /TAU;

    // Prewarped analog corner frequency
    double w_ca = 2.0 * (DEFAULT_SAMPLE_RATE/8) * tan(w_c / (2.0 * (DEFAULT_SAMPLE_RATE/8)));
    double kk = -w_ca / (2.0 * (DEFAULT_SAMPLE_RATE/8));
    double z1 = -1.0;
    double p1 = (1.0 + kk) / (1.0 - kk);
    double b0 = -kk / (1.0 - kk);

    double btaps[2] = { b0 * 1, b0 * -z1 };
    double ataps[2] = {      1,      -p1 };

    double dt = (-1)/(240e3 *  50e-6); 
    double alpha = 1-exp(dt);
    double dt2 = (-1)/(240e3 * 8.376575952e-6);
    double alpha2 = 1 - exp(dt2);

    printf("dt: %f alpha:%f alpha2:%f\\n", dt, alpha, alpha2);


    samples = read_samples(NUM_SAMPLES);

    for(i=0; i< NUM_SAMPLES*2;i++){
        double temp = (double)(*(samples + i*sizeof(uint8_t)) - 127.5);
        d_samples[i] = temp;
    }

    for(i=0; i< NUM_SAMPLES*2;i++){
        fwrite(samples+ i * sizeof(uint8_t), 1,1,rf);
    }



    low_pass(d_samples, filtered, NUM_SAMPLES*2);

    // for(j=0;j<(NUM_SAMPLES*2);j++){
    //     fprintf(fout1, "%f\n", d_samples[j]); 
    // }

    

    for(i=0,j=0;i<(NUM_SAMPLES*2);j++){
        IQ_BUF[j] = filtered[i] + filtered[i+1] * I;
        
        i= i+16;

        //printf("%d\n", j);

        if(j>1){
            demodulated_samples[j-1] = carg(IQ_BUF[j] * conj(IQ_BUF[j-1])) * (((DEFAULT_SAMPLE_RATE)/DECIMATION_FACTOR)/(2*PI*FREQ_DEVIATION)) ;
            //printf("%d - %f\n", j-1,  demodulated_samples[j-1]);
            fprintf(demodulated, "%f\n", demodulated_samples[j-1]);
        }



        // if (j==(BUF_LENGHT - 1)){
        //     Fft_transform(IQ_BUF, BUF_LENGHT,false);
        //     for(k=0;k<BUF_LENGHT;k++){
        //         average[k] = average[k] + pow(cabs(IQ_BUF[k]), 2.0);
        //         if(cnt == COUNT - 1){
        //             average[k] = average[k] / COUNT;
        //         }
        //     }
        //     cnt++;
        //     j=-1;
        // }

        //printf("%d\n", j);
    }

    RC_filter(demodulated_samples, deEmphased_samples, (NUM_SAMPLES/DECIMATION_FACTOR), alpha);

    for (int i = 0; i < (NUM_SAMPLES/DECIMATION_FACTOR); i++)
    {
        fprintf(first_filtered, "%f\n", deEmphased_samples[i]);
    }

    for (int i = 0, j=0; i < (NUM_SAMPLES/DECIMATION_FACTOR); j++)
    {
        deEmphased_samples2[j] = deEmphased_samples[i];
        i = i+ 5;
    }
    
    

    //RC_filter(deEmphased_samples, deEmphased_samples2, (NUM_SAMPLES/DECIMATION_FACTOR), alpha2);
    low_pass2(deEmphased_samples2, deEmphased_samples3, (NUM_SAMPLES/(DECIMATION_FACTOR*5)));
    //IIRFloat(btaps, ataps, demodulated_samples, deEmphased_samples, (NUM_SAMPLES/DECIMATION_FACTOR), 2);

    //low_pass2(deEmphased_samples, (NUM_SAMPLES/DECIMATION_FACTOR));

    //return 0;


    // double max = 0.0;
    // for(i=0; i<(NUM_SAMPLES/DECIMATION_FACTOR); i++){
    //     if(deEmphased_samples[i] < 0){
    //         if(deEmphased_samples[i] * -1.0 > max)
    //             max = deEmphased_samples[i] * -1.0;
    //     }
    //     else if(deEmphased_samples[i] > max)
    //         max = deEmphased_samples[i];
    // }

    //printf("max: %f\n", max);

    // if(!FFT_plot(deEmphased_samples2, (NUM_SAMPLES/DECIMATION_FACTOR), 4096)){
    //     printf("Error in plot\n");
    // }


    for(i=0; i<(NUM_SAMPLES/(DECIMATION_FACTOR*5));  i++){
        decimated_samples[i] = (float) deEmphased_samples3[i];
        fwrite(&decimated_samples[i], 4 , 1,  fout2);
        fprintf(out2, "%f\n", decimated_samples[i]);
        //printf("%f\n", decimated_samples[j]);
    }

    //sf_write_float(file_snd, decimated_samples, (sf_count_t) (NUM_SAMPLES/DECIMATION_FACTOR)/5 );

    // for(j=0;j<(NUM_SAMPLES/DECIMATION_FACTOR);j++){
    //     fprintf(fout1, "%f\n", demodulated_samples[j]); 
    // }


    //sf_close(file_snd);

    fclose(fout2);
    fclose(rf);
    fclose(fout1);
    fclose(first_filtered);
    fclose(demodulated);
    free(IQ_BUF);
    free(average);
    free(demodulated_samples);
    free(deEmphased_samples);
    free(deEmphased_samples2);
    free(deEmphased_samples3);
    free(decimated_samples);
    free(d_samples);
    free(samples);
    return 0;

}