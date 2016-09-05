#! /bin/bash

for task in loadwav loadmp3 zcr resample stft mfcc; do
    ./essentia-bench $task | tail -n 1
done
