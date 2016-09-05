#! /bin/bash

for task in loadwav loadmp3 zcr resample stft mfcc; do
    ./essentia-bench.py $task | tail -n 1
done
