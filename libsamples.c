#include <stdio.h>
#include <stdlib.h>
#include "rtl-sdr.h"
#include <string.h>
#include <inttypes.h>
#include "libsamples.h"

#define DEFAULT_SAMPLE_RATE		2000000
#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(256 * 16384)

static int do_exit = 0,i=0,j=0;
static uint32_t bytes_to_read = 0;
static rtlsdr_dev_t *dev = NULL;

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

static void rtlsdr_callback(uint8_t *buf, uint32_t len, void* ctx)
{
	if (ctx) {
		if (do_exit){
			j=0;
			return;
		}

		//fprintf(stdout, "%p\n", (unsigned char *) ctx);
		
		uint8_t * data = (uint8_t *) ctx;

		//fprintf(stdout, "%p\n", data);

		if ((bytes_to_read > 0) && (bytes_to_read < len)) {
			len = bytes_to_read;
			do_exit = 1;
			rtlsdr_cancel_async(dev);
		}

		

		for(i=0;i<len;i++,j++){
			*(data + j*sizeof(uint8_t)) = *(buf + i*sizeof(uint8_t));
			//fprintf(stdout, "%c - %c\n", data[j], buf[i]);
		}

		if (bytes_to_read > 0)
			bytes_to_read -= len;
	}
}


unsigned char *read_samples(const int number_of_samples)
{
	int r;
	int firstTime = 1;
	int gain = 20;
	int ppm_error = 0;
	int sync_mode = 0;
	int dev_index = 0;
	int dev_given = 0;
	uint32_t frequency = 90e6 - 250000;
	uint32_t samp_rate = DEFAULT_SAMPLE_RATE;
	uint32_t out_block_size = DEFAULT_BUF_LENGTH;

    bytes_to_read = number_of_samples*2;
    uint8_t *samples;

	if(firstTime == 1){

		samples = malloc(bytes_to_read* sizeof(uint8_t));
		fprintf(stdout, "Initial: %p\n", samples);

		/* Device initialization */

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
		firstTime = 0;
	}

	/* Device setting */

	rtlsdr_set_sample_rate(dev, DEFAULT_SAMPLE_RATE);
	rtlsdr_set_center_freq(dev, frequency);
	rtlsdr_set_tuner_gain_mode(dev, 1);
	if(rtlsdr_set_tuner_gain(dev, 400) == 1){printf("error in setting gain\n"); exit(-1);}
	rtlsdr_set_freq_correction(dev, ppm_error);
    r = rtlsdr_reset_buffer(dev);


	/* Reading samples */

    r = rtlsdr_read_async(dev, rtlsdr_callback, samples,
				      0, out_block_size);


    rtlsdr_close(dev);
    return samples;

}
