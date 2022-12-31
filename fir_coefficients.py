import scipy.signal as signal

fs = 1920000
b = signal.firwin(64,2*200e3/float(fs))

print(b)