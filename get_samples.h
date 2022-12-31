#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <rtl-sdr.h>

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

unsigned char *read_samples(const uint32_t number_of_samples, struct dongle_parameters* dp);