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

- [Capture and decode FM radio](https://witestlab.poly.edu/blog/capture-and-decode-fm-radio/)
- [How to capture raw IQ data from a RTL-SDR dongle and FM demodulate with MATLAB](https://www.aaronscher.com/wireless_com_SDR/RTL_SDR_AM_spectrum_demod.html)
- [Site to project FIR low pass filter](https://fiiir.com/)
- Book: *Digital Signal Processing 101: Everything You Need to Know to Get Started, Michael Parker, 2010*