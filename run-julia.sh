#! /bin/bash

MEDLEYDB=~/workspace/medleydb

for track in `ls $MEDLEYDB/*.wav`; do
    for task in loadwav loadmp3 zcr resample stft mfcc stretch; do
        ./julia-bench.jl $track $task | tail -n 1
    done
done
