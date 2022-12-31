import numpy as np
try:                from itertools import izip
except ImportError: izip = zip
import matplotlib.pyplot as plt

bytes = np.fromfile("./debug/IQdata.dat",dtype=np.double)
#print(type(bytes[1]/127.5 - 1))
#iq = [complex(i/(255/2) - 1, q/(255/2) - 1) for i, q in izip(bytes[::2], bytes[1::2])]
#print(type(iq[1]))
Fs=1920000
print(bytes)

plt.psd(bytes, NFFT=2048, Fs=240000)
plt.show()