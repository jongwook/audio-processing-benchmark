#! /usr/bin/env python2.7

import sys
import essentia
import essentia.standard
from time import time
import numpy as np

def benchmark(block, repeats):
    block()
    t0 = time()
    for i in range(repeats):
        print(i)
        block()
    t1 = time()
    average = (t1 - t0) / repeats
    print("Essentia-Python\t%s\t%f\t%d" % (block.__name__, average, repeats))

def nframes(length, framesize, hopsize):
    return (length - framesize) // hopsize + 1

_data = None
framesize = 1024
hopsize = 256
nbins = (framesize >> 1) + 1

def data():
    global _data
    if _data is None:
        _data = loadwav()
    return _data

def loadwav():
    return essentia.standard.MonoLoader(filename = 'data/Phoenix_ScotchMorris_MIX.wav')()

def loadmp3():
    return essentia.standard.MonoLoader(filename = 'data/Phoenix_ScotchMorris_MIX.mp3')()

def zcr():
    audio = data()
    fc = essentia.standard.FrameCutter(frameSize = framesize, hopSize = hopsize)
    zcr = essentia.standard.ZeroCrossingRate()
    rates = np.zeros((nframes(len(audio), framesize, hopsize) + 10,), dtype=np.float32)
    i = 0

    while True:
        frame = fc(audio)
        if len(frame) == 0:
            break
        rates[i] = zcr(frame)
        i += 1

    return rates


def resample():
    audio = data()
    resample = essentia.standard.Resample(inputSampleRate = 44100, outputSampleRate = 48000)
    return resample(audio)

def stft():
    audio = data()
    fc = essentia.standard.FrameCutter(frameSize = framesize, hopSize = hopsize)
    w = essentia.standard.Windowing(size = framesize, type = "hann")
    fft = essentia.standard.FFT(size = framesize)
    spectrogram = np.zeros((nbins, nframes(len(audio), framesize, hopsize) + 10), dtype=np.complex64)
    i = 0

    while True:
        frame = fc(audio)
        if len(frame) == 0:
            break
        windowed = w(frame)
        spectrum = fft(windowed)
        spectrogram[:, i] = spectrum

    return spectrogram


def mfcc():
    audio = data()
    fc = essentia.standard.FrameCutter(frameSize = framesize, hopSize = hopsize)
    w = essentia.standard.Windowing(size = framesize, type = "hann")
    spec = essentia.standard.Spectrum(size = framesize)
    mfcc = essentia.standard.MFCC(highFrequencyBound = 22050, inputSize = nbins, numberBands = 128, numberCoefficients = 20)

    mfcc_matrix = np.zeros((20, nframes(len(audio), framesize, hopsize) + 1), dtype=np.complex64)
    i = 0

    while True:
        frame = fc(audio)
        if len(frame) == 0:
            break
        windowed = w(frame)
        spectrum = spec(windowed)
        bands, mfcc_vector = mfcc(spectrum)
        mfcc_matrix[:, i] = mfcc_vector

    return mfcc_matrix

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
