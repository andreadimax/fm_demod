#include <stdio.h>
#include <rtl-sdr.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "chan.h"
#include <signal.h>

#define DEFAULT_SAMPLE_RATE		1920000
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)

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
            printf("Should print %c\n", buf[i]);
            chan_send(channel, (void *) (uintptr_t) buf[i]);
        }
        		

		return;
        
	}
}

void *receiver(void* chan){

    void *data;

    while (chan_recv((chan_t*) chan, &data) == 0)
    {
        printf("Received %c\n", (uint8_t) data);
    }

    printf("Receiver ending...\n");

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