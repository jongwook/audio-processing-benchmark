#! /usr/bin/env python2.7

import sys
import essentia
import essentia.standard
from time import time
import numpy as np

def benchmark(filename, block, repeats):
    block(filename)

    times = np.zeros(repeats)
    for i in range(repeats):
        print(i)
        t0 = time()
        block(filename)
        t1 = time()
        times[i] = t1 - t0

    print("Essentia-Python\t%s\t%s\t%f\t%f\t%d" % (filename, block.__name__, np.mean(times), np.std(times, ddof = 1), repeats))

_filename = None
_data = None

def data(filename):
    global _data, _filename
    if _data is None or _filename != filename:
        _data = loadwav(filename)
        _filename = filename
    return _data

def nframes(length, framesize, hopsize):
    return (length - framesize) // hopsize + 1

framesize = 1024
hopsize = 256
nbins = (framesize >> 1) + 1

def loadwav(filename):
    return essentia.standard.MonoLoader(filename = filename)()

def loadmp3(filename):
    return essentia.standard.MonoLoader(filename = filename.replace(".wav", ".mp3"))()

def zcr(filename):
    audio = data(filename)
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


def resample(filename):
    audio = data(filename)
    resample = essentia.standard.Resample(inputSampleRate = 44100, outputSampleRate = 48000)
    return resample(audio)

def stft(filename):
    audio = data(filename)
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


def mfcc(filename):
    audio = data(filename)
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
