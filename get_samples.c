#include "get_samples.h"

#include <signal.h>

#define DEFAULT_SAMPLE_RATE		1920000
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)


static rtlsdr_dev_t *dev = NULL; // pointer to the dongle
static uint32_t bytes_to_read = 0; //# of bytes to read could be > than maximum buf length
uint8_t firstCall = 1;
uint8_t* usr_buf = NULL;

void sig_handler(int signum){

	printf("Read signal!");
	rtlsdr_cancel_async(dev);

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

		if (firstCall)
		{
			usr_buf = (uint8_t *) ctx;
			firstCall = 0;
		}	

        //Copying data
        // for(;i<len;i++){
		// 	*(data + i*sizeof(uint8_t)) = *(buf + i*sizeof(uint8_t));
		// }
		memcpy(usr_buf,buf,len);
		usr_buf = usr_buf + len * sizeof(uint8_t);

		bytes_to_read -= len;

		if (bytes_to_read  == 0)
		{
			rtlsdr_cancel_async(dev);
			return;
		}		

		return;
        
	}
}

/**
 * @brief API exposed to rest of the program to get the samples
 * 
 * @param number_of_samples 
 * @param dp Data structure with dongle parameters
 * @return An array dynamicall allocated of (number_of_samples*2) unsigned char* samples
 */
unsigned char *read_samples(const uint32_t number_of_samples, struct dongle_parameters* dp)
{

    if (dp == NULL)
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


	/* Reading samples */

    r = rtlsdr_read_async(dev, 
                          rtlsdr_callback, 
                          samples,
                          3, 
                          MAXIMAL_BUF_LENGTH);


    rtlsdr_close(dev);

    return samples;

}