#! /usr/bin/env python

import sys
import librosa
import resampy
import numpy as np
from time import time

def benchmark(filename, block, repeats):
    block(filename)

    times = np.zeros(repeats)
    for i in range(repeats):
        print(i)
        t0 = time()
        block(filename)
        t1 = time()
        times[i] = t1 - t0

    print("Librosa\t%s\t%s\t%f\t%f\t%d" % (filename, block.__name__, np.mean(times), np.std(times, ddof = 1), repeats))


_filename = None
_data = None

def data(filename):
    global _data, _filename
    if _data is None or _filename != filename:
        _data = loadwav(filename)
        _filename = filename
    return _data


def loadwav(filename):
    return librosa.load(filename, sr = None)

def loadmp3(filename):
    return librosa.load(filename.replace(".wav", ".mp3"), sr = None)

def zcr(filename):
    audio, fs = data(filename)
    return librosa.feature.zero_crossing_rate(audio, 1024, 256)

def resample(filename):
    audio, fs = data(filename)
    return resampy.resample(audio, fs, 48000, filter = 'sinc_window', num_zeros = 44, precision = 9, rolloff = 0.9)

def stft(filename):
    audio, fs = data(filename)
    return librosa.stft(audio, n_fft = 1024, hop_length = 256)

def mfcc(filename):
    audio, fs = data(filename)
    return librosa.feature.mfcc(audio, fs, n_fft = 1024, hop_length = 256)

def stretch(filename):
    audio, fs = data(filename)
    return librosa.effects.time_stretch(audio, 2.0)

if __name__ == "__main__":
    try:
        filename = sys.argv[1]
        task = eval(sys.argv[2])
        assert callable(task)

        if len(sys.argv) >= 4:
            repeats = int(sys.argv[3])
        else:
            repeats = 100
    except:
        print("Usage: %s [filename] [benchmark name] [number of repeats; 100 by default]" % sys.argv[0])

    benchmark(filename, task, repeats)
