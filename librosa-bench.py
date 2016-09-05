#! /usr/bin/env python

import sys
import librosa
import resampy
from time import time

def benchmark(block, repeats):
    block()
    t0 = time()
    for i in range(repeats):
        print(i)
        block()
    t1 = time()
    average = (t1 - t0) / repeats
    print("Librosa\t%s\t%f\t%d" % (block.__name__, average, repeats))

_data = None

def data():
    global _data
    if _data is None:
        _data = loadwav()
    return _data

def loadwav():
    return librosa.load("data/Phoenix_ScotchMorris_MIX.wav", sr = None)

def loadmp3():
    return librosa.load("data/Phoenix_ScotchMorris_MIX.mp3", sr = None)

def zcr():
    audio, fs = data()
    return librosa.feature.zero_crossing_rate(audio, 1024, 256)

def resample():
    audio, fs = data()
    return resampy.resample(audio, fs, 48000, filter = 'sinc_window', num_zeros = 44, precision = 9, rolloff = 0.9)

def stft():
    audio, fs = data()
    return librosa.stft(audio, n_fft = 1024, hop_length = 256)

def mfcc():
    audio, fs = data()
    return librosa.feature.mfcc(audio, fs, n_fft = 1024, hop_length = 256)

def stretch():
    audio, fs = data()
    return librosa.effects.time_stretch(audio, 2.0)

if __name__ == "__main__":
    try:
        task = eval(sys.argv[1])
        assert callable(task)

        if len(sys.argv) >= 3:
            repeats = int(sys.argv[2])
        else:
            repeats = 100

        benchmark(task, repeats)
    except:
        print("Usage: %s [benchmark name] [number of repeats; 100 by default]" % sys.argv[0])
