#! /bin/bash

for task in loadwav loadmp3 zcr resample stft mfcc stretch; do
    ./julia-bench.jl $task | tail -n 1
done
