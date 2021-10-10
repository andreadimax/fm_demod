#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fft-complex.h"
#include <complex.h>


// Private function prototypes
static size_t reverse_bits(size_t val, int width);
static void *memdup(const void *src, size_t n);


bool Fft_transform(double complex vec[], size_t n, bool inverse) {
	if (n == 0)
		return true;
	else if ((n & (n - 1)) == 0)  // Is power of 2
		return Fft_transformRadix2(vec, n, inverse);
	else  // More complicated algorithm for arbitrary sizes
		return Fft_transformBluestein(vec, n, inverse);
}


bool Fft_transformRadix2(double complex vec[], size_t n, bool inverse) {
	// Length variables
	int levels = 0;  // Compute levels = floor(log2(n))
	for (size_t temp = n; temp > 1U; temp >>= 1)
		levels++;
	if ((size_t)1U << levels != n)
		return false;  // n is not a power of 2
	
	// Trigonometric tables
	if (SIZE_MAX / sizeof(double complex) < n / 2)
		return false;
	double complex *exptable = malloc((n / 2) * sizeof(double complex));
	if (exptable == NULL)
		return false;
	for (size_t i = 0; i < n / 2; i++)
		exptable[i] = cexp((inverse ? 2 : -2) * M_PI * i / n * I);
	
	// Bit-reversed addressing permutation
	for (size_t i = 0; i < n; i++) {
		size_t j = reverse_bits(i, levels);
		if (j > i) {
			double complex temp = vec[i];
			vec[i] = vec[j];
			vec[j] = temp;
		}
	}
	
	// Cooley-Tukey decimation-in-time radix-2 FFT
	for (size_t size = 2; size <= n; size *= 2) {
		size_t halfsize = size / 2;
		size_t tablestep = n / size;
		for (size_t i = 0; i < n; i += size) {
			for (size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep) {
				size_t l = j + halfsize;
				double complex temp = vec[l] * exptable[k];
				vec[l] = vec[j] - temp;
				vec[j] += temp;
			}
		}
		if (size == n)  // Prevent overflow in 'size *= 2'
			break;
	}
	
	free(exptable);
	return true;
}


bool Fft_transformBluestein(double complex vec[], size_t n, bool inverse) {
	bool status = false;
	
	// Find a power-of-2 convolution length m such that m >= n * 2 + 1
	size_t m = 1;
	while (m / 2 <= n) {
		if (m > SIZE_MAX / 2)
			return false;
		m *= 2;
	}
	
	// Allocate memory
	if (SIZE_MAX / sizeof(double complex) < n || SIZE_MAX / sizeof(double complex) < m)
		return false;
	double complex *exptable = malloc(n * sizeof(double complex));
	double complex *avec = calloc(m, sizeof(double complex));
	double complex *bvec = calloc(m, sizeof(double complex));
	double complex *cvec = malloc(m * sizeof(double complex));
	if (exptable == NULL || avec == NULL || bvec == NULL || cvec == NULL)
		goto cleanup;
	
	// Trigonometric tables
	for (size_t i = 0; i < n; i++) {
		uintmax_t temp = ((uintmax_t)i * i) % ((uintmax_t)n * 2);
		double angle = (inverse ? M_PI : -M_PI) * temp / n;
		exptable[i] = cexp(angle * I);
	}
	
	// Temporary vectors and preprocessing
	for (size_t i = 0; i < n; i++)
		avec[i] = vec[i] * exptable[i];
	bvec[0] = exptable[0];
	for (size_t i = 1; i < n; i++)
		bvec[i] = bvec[m - i] = conj(exptable[i]);
	
	// Convolution
	if (!Fft_convolve(avec, bvec, cvec, m))
		goto cleanup;
	
	// Postprocessing
	for (size_t i = 0; i < n; i++)
		vec[i] = cvec[i] * exptable[i];
	status = true;
	
	// Deallocation
cleanup:
	free(exptable);
	free(avec);
	free(bvec);
	free(cvec);
	return status;
}


bool Fft_convolve(const double complex xvec[], const double complex yvec[],
		double complex outvec[], size_t n) {
	
	bool status = false;
	if (SIZE_MAX / sizeof(double complex) < n)
		return false;
	double complex *xv = memdup(xvec, n * sizeof(double complex));
	double complex *yv = memdup(yvec, n * sizeof(double complex));
	if (xv == NULL || yv == NULL)
		goto cleanup;
	
	if (!Fft_transform(xv, n, false))
		goto cleanup;
	if (!Fft_transform(yv, n, false))
		goto cleanup;
	for (size_t i = 0; i < n; i++)
		xv[i] *= yv[i];
	if (!Fft_transform(xv, n, true))
		goto cleanup;
	for (size_t i = 0; i < n; i++)  // Scaling (because this FFT implementation omits it)
		outvec[i] = xv[i] / n;
	status = true;
	
cleanup:
	free(xv);
	free(yv);
	return status;
}


static size_t reverse_bits(size_t val, int width) {
	size_t result = 0;
	for (int i = 0; i < width; i++, val >>= 1)
		result = (result << 1) | (val & 1U);
	return result;
}


static void *memdup(const void *src, size_t n) {
	void *dest = malloc(n);
	if (n > 0 && dest != NULL)
		memcpy(dest, src, n);
	return dest;
}

bool FFT_plot(double vec[], size_t n, const int BUF_LENGHT){

	if(n < BUF_LENGHT){
		return false;
	}

	int cnt=n,i,j,k;
	double complex *samples;
	double complex *IQ_BUF;
	double *average = malloc(BUF_LENGHT * sizeof(double));
	FILE* out = fopen("output.dat", "w");
	FILE* gp1 = popen("gnuplot -persist", "w");

	if(n%BUF_LENGHT != 0){
		cnt=n+1;
		while (cnt%BUF_LENGHT != 0)
		{
			cnt++;
		}
		samples = malloc(cnt * sizeof(double complex));
		memset( samples, 0, sizeof(cnt * sizeof(double complex)));
	}
	else{
		samples = malloc(n * sizeof(double complex));
		memset( samples, 0, sizeof(n * sizeof(double complex)));
	}

	for ( i = 0; i < n; i++)
	{
		samples[i] = vec[i] + 0 * I;
		//printf("%f\n", vec[i]);
	}

	int nBlocks = (int) cnt/BUF_LENGHT;

	for (i=0, IQ_BUF = (samples + i*BUF_LENGHT*sizeof(double complex)); i < 1; i++)
	{
		for(j=0; j<BUF_LENGHT; j++){
			printf("%f\n",creal(IQ_BUF[j]));
		}

		//printf("\n");

		if(!Fft_transform(IQ_BUF, BUF_LENGHT, false)){
			return false;
		}
		for (k = 0; k < BUF_LENGHT; k++)
		{
			average[k] = average[k] + cabs(IQ_BUF[k]);
			printf("%f - ", average[k]);
		}

		printf("\n----------------------\n");
		
		if (i == nBlocks - 1)
		{
			average[k] = average[k] / nBlocks;
		}
	}

	for(i=0; i<BUF_LENGHT;i++){
		//printf("%f\n", average[i]);
		fwrite(&average[i], 4,1,out);
	}
	free(samples);
	free(average);

	fclose(out);
	fprintf(gp1, "plot [0:%d] 'output.dat' w filledcurve x1 lw 2 lc 'red'\n", BUF_LENGHT);
	pclose(gp1);
	return true;
}