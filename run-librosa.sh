#! /bin/bash

MEDLEYDB=~/workspace/medleydb

for track in `ls $MEDLEYDB/*.wav`; do
    for task in loadwav loadmp3 zcr resample stft mfcc stretch; do
        ./librosa-bench.py $track $task 100 | tail -n 1
    done
done
