default:
	gcc -o sp1 demodulator.c low_pass.c libsamples.c fft-complex.c -lm -lrtlsdr -lsndfile
execute:
	./sp1
ce:
	gcc -o sp1 demodulator.c low_pass.c libsamples.c fft-complex.c -lm -lrtlsdr -lsndfile
	./sp1
	aplay audio.raw -r 48000 -f FLOAT_LE -t raw -c 1

	