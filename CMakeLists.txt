cmake_minimum_required(VERSION 3.22)
project(fm_demod)

add_executable(fm_demod demodulator.c filters.c filters.h get_samples.c get_samples.h)

target_link_libraries(fm_demod rtlsdr m)