compile:
	gcc -o sp1 demodulator.c low_pass.c libsamples.c fft-complex.c -lm -lrtlsdr -lsndfile
execute:
	./sp1
ce:
	gcc -o sp1 demodulator.c low_pass.c libsamples.c fft-complex.c -lm -lrtlsdr -lsndfile
	./sp1
	