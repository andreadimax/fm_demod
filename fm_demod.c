#include "rtl-sdr.h"
#include <pthread.h>
#include <stdio.h>
#include <complex.h>
#include <string.h>
#include <stdlib.h>
#include <math.h> 
#include <time.h>
#include "portaudio.h"

#define DEFAULT_SAMPLE_RATE 1920000
#define NUM_SAMPLES 1920000
#define FRAMES_PER_BUFFER 512
#define FREQ_DEVIATION 75000

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

uint8_t firstCall = 1;
static rtlsdr_dev_t *dev = NULL; // pointer to the dongle

struct shared_data{
    uint8_t index;
	float** samples;
	pthread_mutex_t mutex;
};

struct dongle_parameters{
    int gain;
	int ppm_error;
	int sync_mode;
	int dev_index;
	int dev_given;
	uint32_t frequency;
	uint32_t samp_rate;
};

/**
 * @brief Searches for the USB dongle
 * 
 * @param s 
 * @return int 
 */
int verbose_device_search(char *s)
{
	int i, device_count, device, offset;
	char *s2;
	char vendor[256], product[256], serial[256];

	device_count = rtlsdr_get_device_count();

	if (!device_count) {
		fprintf(stderr, "No supported devices found.\n");
		return -1;
	}

	fprintf(stderr, "Found %d device(s):\n", device_count);

	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
	}

	fprintf(stderr, "\n");

	/* does string look like raw id number */
	device = (int)strtol(s, &s2, 0);
	if (s2[0] == '\0' && device >= 0 && device < device_count) {
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}

	/* does string exact match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strcmp(s, serial) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}

	/* does string prefix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strncmp(s, serial, strlen(s)) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}

	/* does string suffix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		offset = strlen(serial) - strlen(s);
		if (offset < 0) {
			continue;}
		if (strncmp(s, serial+offset, strlen(s)) != 0) {
			continue;}
		device = i;
		fprintf(stderr, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
		return device;
	}

	fprintf(stderr, "No matching devices found.\n");
    
	return -1;
}


/**
 * @brief Callback to get signal samples from dongle
 * 
 * @param buf Buffer where sample read from dongle are saved
 * @param len Length in bytes of the data samples to read
 * @param ctx User buffer where to save samples
 */
static void rtlsdr_callback(uint8_t *buf, uint32_t len, void* ctx)
{
	if (ctx) {	
        memcpy(ctx,buf,len);
	}

	rtlsdr_cancel_async(dev);
	return;        
}

/**
 * @brief API exposed to rest of the program to get the samples
 * 
 * @param number_of_samples 
 * @param dp Data structure with dongle parameters
 * @return An array dynamicall allocated of (number_of_samples*2) unsigned char* samples
 */
static void *read_samples(const uint32_t number_of_samples, uint8_t* samples, struct dongle_parameters* dp)
{

    if (dp == NULL)
    {
        printf("Error: dongle parameters not initialized!\n");
        exit(1);
    }

	if (samples == NULL)
	{
		printf("Error: samples buffer not initialized!\n");
		exit(1);
	}

	int r = 0;

	if (firstCall)
	{
		int dev_index = 0;
		int dev_given = 0;

		/* ----  Device initialization ---- */
		/* -------------------------------- */

		if (!dev_given) {
			dev_index = verbose_device_search("0");
		}

		if (dev_index < 0) {
			fprintf(stderr, "%s\n", "No device found!\n");
			exit(1);
		}

		r = rtlsdr_open(&dev, (uint32_t)dev_index);

		if (r < 0) {
			fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
			exit(1);
		}
		
		/* -------------------------------- */

		/* Device setting */

		rtlsdr_set_sample_rate(dev, dp->samp_rate);
		rtlsdr_set_center_freq(dev, dp->frequency);
		rtlsdr_set_tuner_gain_mode(dev, 0);
		rtlsdr_set_freq_correction(dev, dp->ppm_error);
		r = rtlsdr_reset_buffer(dev);

		firstCall = 0;
	}

	/* Reading samples */

    r = rtlsdr_read_async(dev, 
                          rtlsdr_callback, 
                          samples,
                          1, 
                          number_of_samples * 2);

}

static void fir_filter(float* input_real, float* input_imag, float complex* output_complex, int length)
{

    double acc;     // accumulator for sum
    int n = 0;
    int k = 0;
    int i = 0;

	float* output_real = (float*) malloc(length * sizeof(float));
    float* output_imag = (float*) malloc(length * sizeof(float));
 
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


    free(output_real);
    free(output_imag);

    return;
 
}

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

void *main_thread(void *arg){

	if (arg == NULL)
	{
		printf("Error: shared data struct not inizialized\n");
		exit(1);
	}

	struct shared_data* sd = (struct shared_data*) arg;
	float* fir_input_real = (float*) malloc(NUM_SAMPLES * sizeof(float));
    float* fir_input_imag = (float*) malloc(NUM_SAMPLES * sizeof(float));
	float complex* fir_output = (float complex*) malloc(NUM_SAMPLES * sizeof(float complex));
	float  *demodulated_samples = malloc((NUM_SAMPLES/8)* sizeof(float));
	float  *deEmphased_samples = malloc((NUM_SAMPLES/8)* sizeof(double));
	float complex *decims = (float complex *) malloc(NUM_SAMPLES/8 * sizeof(float complex));
	uint8_t* samples = malloc((NUM_SAMPLES * 2) * sizeof(uint8_t));
	int i,j,counter = 0;

	float d = (DEFAULT_SAMPLE_RATE/8) * 50e-6 ; //for Europe 50e-6, for America 75e-6
    float xx = exp(-1/d);
    float b_coeffs[1] = {1-xx};   
    float a_coeffs[2] = {1,-xx};

    float *y1 = (float*) malloc(NUM_SAMPLES/8 * sizeof(float));

	struct dongle_parameters dp;
	dp.gain = 0;
    dp.ppm_error = 0;
    dp.sync_mode = 0;
    dp.frequency = 100.4e6;
    dp.samp_rate = DEFAULT_SAMPLE_RATE;
	clock_t start, end;
    double cpu_time_used;
	
	pthread_mutex_lock(&(sd->mutex));

	start = clock();

	read_samples(NUM_SAMPLES, samples, &dp);

	for(i=0, j=0; i< NUM_SAMPLES*2;j++){
        fir_input_real[j] = (*(samples + (i)*sizeof(uint8_t)) / (127.5) - 1);
		fir_input_imag[j] = (*(samples + (i+1)*sizeof(uint8_t)) / (127.5) - 1);
        i=i+2;
    }

	fir_filter(fir_input_real,fir_input_imag,fir_output,NUM_SAMPLES);

    //decimating from 1920000 to 240000 Hz (factor 8)
    for (i = 0, j= 0; i < NUM_SAMPLES;j++ )
    {
        decims[j] = fir_output[i];
        i=i+8;
    }

	for (j = 1; j <= NUM_SAMPLES/8; j++)
    {
        demodulated_samples[j-1] = cargf(decims[j] * conj(decims[j-1])) * (((DEFAULT_SAMPLE_RATE)/8)/(2*M_PI*FREQ_DEVIATION));
    }

	// for ( i = 0; i < NUM_SAMPLES/8; i++)
    // {
    //     y1[i] = 0;
    // }
	

	iir_filter(demodulated_samples,y1,NUM_SAMPLES/8,b_coeffs,1,a_coeffs,2);

	for (int i = 0, j=0; i < (NUM_SAMPLES/8); j++)
    {
        deEmphased_samples[j] = y1[i];
        i = i+ 5;
    }

	memcpy(sd->samples[0], deEmphased_samples, NUM_SAMPLES/(8*5)*4);

	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

	printf("Time elapsed: %f\n", cpu_time_used);

	FILE* samples_saved = fopen("audio.raw", "wb");

	for(i=0; i<((NUM_SAMPLES)/(8*5));  i++){
        fwrite(&deEmphased_samples[i], 4 , 1,  samples_saved);
    }

	fclose(samples_saved);

	sd->index = 0;

	pthread_mutex_unlock(&(sd->mutex));

	free(fir_input_real);
    free(fir_input_imag);
	free(fir_output);
	free(demodulated_samples);
	free(deEmphased_samples);
	free(y1);
	free(samples);

}

static int remaining = 48000;
static float* point = NULL;

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    struct shared_data *sd = (struct shared_data *)userData; 
    float *out = (float*)outputBuffer;
    unsigned int i,j;
    (void) inputBuffer; /* Prevent unused variable warning. */


	while (remaining > 0)
	{
		if (remaining <= FRAMES_PER_BUFFER)
		{
			for (i = 0; i < remaining; i++)
			{
				*out++ = *point;
				*out++ = *point++;
			}
		}
		else{
			for (i = 0; i < FRAMES_PER_BUFFER; i++)
			{
				*out++ = *point;
				*out++ = *point++;
			}
		}

		remaining -= FRAMES_PER_BUFFER;

		return paContinue;
		
	}	

	printf("Completed!\n");
    
    return paComplete;
}


void *play_thread(void* arg){

	if (arg == NULL)
	{
		printf("Error: shared data struct not inizialized\n");
		exit(1);
	}

	struct shared_data* sd = (struct shared_data*) arg;

	PaStream *stream;
	PaError err;

	err = Pa_Initialize();
	if( err != paNoError ){
		printf("paErr!\n");
		exit(1);
	}

	err = Pa_OpenDefaultStream( &stream,
                                   0,          /* no input channels */
                                   2,          /* stereo output */
                                   paFloat32,  /* 32 bit floating point output */
                                   48000,
                                   FRAMES_PER_BUFFER,        /* frames per buffer */
                                   patestCallback,
                                   sd );
	pthread_mutex_lock(&(sd->mutex));
	point = sd->samples[0];
	pthread_mutex_unlock(&(sd->mutex));
	Pa_StartStream( stream );
	Pa_Sleep(3*1000);
	Pa_StopStream( stream );
	Pa_CloseStream( stream );
	Pa_Terminate();
  	printf("Test finished.\n");

}

int main(){

	pthread_t thread1, thread2;
	int i;

	float **samples = malloc(4 * sizeof(float*));

	for (i = 0; i < 4; i++)
	{
		samples[i] = malloc((NUM_SAMPLES/(5*8)) * sizeof(float));
	}

	struct shared_data sd;
	sd.index = 0;
	sd.samples = samples;


	pthread_create(&thread1,NULL,main_thread,(void*) &sd);
	pthread_create(&thread2,NULL,play_thread, (void*) &sd);

	pthread_join( thread1, NULL);
    pthread_join( thread2, NULL); 

    printf("Thread 1 returns\n");
    printf("Thread 2 returns\n");

	for (i = 0; i < 4; i++)
	{
		free(samples[i]);
	}

	free(samples);

	rtlsdr_close(dev);
    exit(0);	
}
