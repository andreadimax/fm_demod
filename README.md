# FM Radio demodulator

Hobby personal project to explore the world of Software Defined Radio and Digital Signal Processing and put the signal theory knowledge from university into practice.


I wanted to write a native demodulator in C after studying the necessary components of an FM receiver (filtering, demodulation, etc...) so no external libraries are used (except `librtlsdr` for taking I/Q data from the RTL-SDR dongle, a `Realtek RTL2832U`)

For now only few second of music are demodulated, but I hope to develop this project!

## How to run

If you have a Realtek RTL2832U dongle and ypu have successfully installed on your PC (guide for Linux at this [link](https://www.rtl-sdr.com/tag/install-guide/)) you can try my humble demodulator and listen few seconds of FM radio audio

I worked and tested it with Ubuntu 20.04 and 22.04. 

### Dependencies

- [librtlsdr](https://github.com/steve-m/librtlsdr)

### Compile & run

    mkdir build && cd build
    cmake ..
    ./fm_demod

# Steps

- [x] Build a working demodulator and listen to music!
- [ ] Demodulate and play continuous stream of music (in progress)
- [ ] Add a GUI using PyQT5 (in progress)
- [ ] Add RDS support
- [ ] Add DAB support

# Useful resources from which I took inspiration

