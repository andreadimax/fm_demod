import scipy.signal as signal

f_bw = 200000
Fs = 2000000

lpf = signal.remez(64, [0, f_bw, f_bw+(Fs/2-f_bw)/4, Fs/2], [1,0], Hz=Fs)

for element in lpf:
    print(str(element))