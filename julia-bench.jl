#! /usr/bin/env julia

# install packages if not already installed
packages = Pkg.installed()
haskey(packages, "SampledSignals") || Pkg.add("SampledSignals")
haskey(packages, "LibSndFile") || Pkg.add("LibSndFile")
haskey(packages, "MP3") || Pkg.add("MP3")
haskey(packages, "MusicProcessing") || Pkg.clone("https://github.com/jongwook/MusicProcessing.jl")

using MusicProcessing, MP3, SampledSignals, LibSndFile

"""runs the expression repeatedly and returns the mean elapsed time"""
function benchmark(task, repeats)
    task()
    local t0 = time()
    for i in 1:repeats
        println(i)
        task()
    end
    local t1 = time()
    average = (t1 - t0) / repeats
    repeats = repeats
    println("julia\t$(string(task))\t$average\t$repeats")
end

"""lazily-initialized audio data"""
audio = (() -> begin
    local initialized = false
    local data::SampleBuf
    () -> begin
        if !initialized
            data = map(Float32, mono(loadwav()))
            initialized = true
        end
        data
    end
end)()

loadwav() = load("data/Phoenix_ScotchMorris_MIX.wav")
loadmp3() = load("data/Phoenix_ScotchMorris_MIX.mp3")
zcr() = MusicProcessing.zero_crossing_rate(audio(), 1024, 256)
resample() = MusicProcessing.resample(audio(), 48000Hz)
stft() = MusicProcessing.stft(audio(), 1024, 256)
mfcc() = MusicProcessing.mfcc(audio(), 1024, 256; nmfcc = 20, nmels = 128)
stretch() = MusicProcessing.speedup(audio(), 2.0)

target = ""
repeats = 100

try
    target = eval(Symbol(ARGS[1]))
    if length(ARGS) >= 2
        repeats = parse(Int, ARGS[2])
    end
catch
    println("usage: ./$(Base.source_path()) [benchmark name] [number of repeats; 100 by default]")
end

benchmark(target, repeats)
