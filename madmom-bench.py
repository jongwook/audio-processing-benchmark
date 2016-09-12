#! /usr/bin/env python

import sys
import numpy as np
import madmom
from time import time

def benchmark(block, repeats):
    block()
    t0 = time()
    for i in range(repeats):
        print(i)
        block()
    t1 = time()
    average = (t1 - t0) / repeats
    print("Madmom\t%s\t%f\t%d" % (block.__name__, average, repeats))

_filename = None
_data = None

def data(filename):
    global _data
    if _data is None || _filename != filename:
        _data = loadwav(filename)
        _filename = filename
    return _data

def loadwav(filename):
    return madmom.audio.signal.Signal("data/" + filename + "_MIX.wav", num_channels = 1)

def loadmp3(filename):
    return madmom.audio.signal.Signal("data/" + filename + "_MIX.mp3", num_channels = 1)

def zcr(filename):
    pass

def resample(filename):
    pass

def stft(filename):
    signal = data()
    fs = madmom.audio.signal.FramedSignal(signal, frame_size = 1024, hop_size = 256)
    return madmom.audio.stft.STFT(fs)

def mfcc(filename):
    signal = data()
    fs = madmom.audio.signal.FramedSignal(signal, frame_size = 1024, hop_size = 256)
    stft = madmom.audio.stft.STFT(fs)
    spec = madmom.audio.spectrogram.Spectrogram(stft)

    return madmom.audio.MFCC(audio, fs, num_bands = 128, fmin = 0, fmax = 22050)

def stretch(filename):
    audio, fs = data()
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

    benchmark(task, repeats)
