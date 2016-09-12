#! /bin/bash

MEDLEYDB=~/workspace/medleydb

for track in `ls $MEDLEYDB/*.wav`; do
    for task in loadwav loadmp3 zcr resample stft mfcc; do
        ./essentia-bench.py $track $task | tail -n 1
    done
done
