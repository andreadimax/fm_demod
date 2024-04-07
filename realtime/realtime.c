#include <stdio.h>
#include <rtl-sdr.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "chan.h"
#include <signal.h>
#include <complex.h>
#include <math.h>

#define DEFAULT_SAMPLE_RATE		1920000
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)

#define ORDER 39
#define CHUNK 40
#define FIRST_DECIMATION_FACTOR 8
#define SECOND_DECIMATION_FACTOR 5
#define FREQ_DEVIATION 75000

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

struct dongle_parameters{
    int gain;
	int ppm_error;
	int sync_mode;
	int dev_index;
	int dev_given;
	uint32_t frequency;
	uint32_t samp_rate;
	//uint32_t out_block_size;
};

chan_t* channel;


static rtlsdr_dev_t *dev = NULL; // pointer to the dongle
static uint32_t bytes_to_read = 0; //# of bytes to read could be > than maximum buf length
uint8_t firstCall = 1;
uint8_t* usr_buf = NULL;

void sig_handler(int signum){

	printf("Read signal!");
	rtlsdr_cancel_async(dev);

    chan_close(channel);

	return;
}

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

static int do_exit = 0;

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

		printf("Len %d\n", len);

		for (int i = 0; i < len; i++)
        {
            chan_send(channel, (void *) (uintptr_t) buf[i]);
        }
        		

		return;
        
	}
}

static unsigned char initial_counter = 0;
static unsigned char sample_counter = 0;

static unsigned char i_samples[CHUNK];
static unsigned char q_samples[CHUNK];
static float i_output[CHUNK];
static float q_output[CHUNK];

static float complex first_decimation_output[CHUNK / FIRST_DECIMATION_FACTOR];
static float demodulated_samples[CHUNK / FIRST_DECIMATION_FACTOR];


void process(unsigned char sample)
{
	//filling buffer at start
	while (initial_counter != CHUNK)
	{
		// I sample
		if (initial_counter % 2 == 0)
		{
			i_samples[sample_counter] = sample;
		}
		// Q sample
		else
		{
			q_samples[sample_counter++] = sample;
		}

        initial_counter++;

		return;
		
	}

    initial_counter = 0;

    /* ----------------- FIR FILTER ----------------- */
    /* ---------------------------------------------- */

    double acc_real, acc_imag;     // accumulator for sum
    int n = 0;
    int k = 0;
    int i = 0;


    // apply the filter to each sample of each signal (I and Q)
    for (n = 0; n < CHUNK; n++) 
    {
        acc_real = 0;
		acc_imag = 0;

        for ( k = 0; k < ORDER; k++ ) {
            if(n-k >= 0 && n-k <=(CHUNK-1)){
                // I signal
                acc_real += fir_coeffs[k] * i_samples[n-k];

                // Q signal 
				acc_imag += fir_coeffs[k] * q_samples[n-k];
            }
        }

        i_output[n] = acc_real;
		q_output[n] = acc_imag;

    }

    /* ------------- END FIR FILTER ----------------- */
    /* ---------------------------------------------- */

	/* ------------- DEMODULATION ------------- */
    /* ---------------------------------------- */

    /*
        Decimating of a factor FIRST_DECIMATION_FACTOR
        so that we pass to sampling frequency DEFAULT_SAMPLE_RATE / FIRST_DECIMATION_FACTOR
    */
    for (int i = 0, j = 0; i < CHUNK; j++, i += FIRST_DECIMATION_FACTOR)
    {
        first_decimation_output[j] = i_output[i] + I * q_output[i];
    }
    

    //Demodulating using polar discrimination factor
    for (int j = 1; j <= CHUNK / FIRST_DECIMATION_FACTOR; j++)
    {
        demodulated_samples[j-1] = cargf(first_decimation_output[j] * conj(first_decimation_output[j-1])) * (((DEFAULT_SAMPLE_RATE)/FIRST_DECIMATION_FACTOR)/(2*M_PI*FREQ_DEVIATION));
    }

    /* --------- END DEMODULATION --------- */
    /* ------------------------------------ */

    fwrite(&demodulated_samples[0], 4, 1, stdout);

    //printf("%.6f | ", demodulated_samples[0]);

}


void *receiver(void* chan){

    void *data;

    while (chan_recv((chan_t*) chan, &data) == 0)
    {
        //printf("Received %c\n", (uint8_t) data);
        process((unsigned char) data);
    }

}


int main()
{


    //initializing dongle parameters
    struct dongle_parameters *dp = malloc(sizeof(struct dongle_parameters));
    dp->gain = 0;
    dp->ppm_error = 0;
    dp->sync_mode = 0;
    dp->frequency = 91.8e6;
    dp->samp_rate = DEFAULT_SAMPLE_RATE;

	memset(i_samples, 0, CHUNK);
	memset(q_samples, 0, CHUNK);
	memset(i_output, 0, sizeof(float) * (CHUNK));
	memset(q_output, 0, sizeof(float) * (CHUNK));
    memset(demodulated_samples, 0, sizeof(float) * (CHUNK / FIRST_DECIMATION_FACTOR));

    channel = chan_init(0);

    if (dp == NULL || channel == NULL)
    {
        printf("Error: dongle parameters not initialized!\n");
        exit(1);
    }

    // if (dp->out_block_size % 16384 != 0)
    // {
    //     printf("Error: number of samples must be a multiple of 16384\n");
    //     exit(1);
    // }
    
    bytes_to_read = MAXIMAL_BUF_LENGTH * 3;

	int n = bytes_to_read / MAXIMAL_BUF_LENGTH;

	int r = 0;
	int dev_index = 0;
	int dev_given = 0;

    //buffer where to save samples
    unsigned char *samples = NULL;
    samples = (unsigned char*) malloc(MAXIMAL_BUF_LENGTH * 3 * sizeof(unsigned char));

    if (samples == NULL)
    {
        printf("Error in samples buffer allocation\n");
        exit(1);
    }    

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
	//if(rtlsdr_set_tuner_gain(dev, 400) == 1){printf("error in setting gain\n"); exit(-1);}
	rtlsdr_set_freq_correction(dev, dp->ppm_error);
    r = rtlsdr_reset_buffer(dev);

    pthread_t thread_recv;
    pthread_create(&thread_recv, NULL, receiver, (void*) channel);


	/* Reading samples */

    r = rtlsdr_read_async(dev, 
                          rtlsdr_callback, 
                          samples,
                          3, 
                          MAXIMAL_BUF_LENGTH);


    pthread_join(thread_recv, NULL);

    printf("Hello\n");



    rtlsdr_close(dev);

    return 0;

}