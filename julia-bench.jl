#! /usr/bin/env julia

# install packages if not already installed
packages = Pkg.installed()
haskey(packages, "SampledSignals") || Pkg.add("SampledSignals")
haskey(packages, "LibSndFile") || Pkg.add("LibSndFile")
haskey(packages, "MP3") || Pkg.add("MP3")
haskey(packages, "MusicProcessing") || Pkg.clone("https://github.com/jongwook/MusicProcessing.jl")

using MusicProcessing, MP3, SampledSignals, LibSndFile

"""runs the expression repeatedly and returns the mean elapsed time"""
function benchmark(filename, task, repeats)
    task(filename)
    times = zeros(repeats)
    for i in 1:repeats
        println(i)
        local t0 = time()
        task(filename)
        local t1 = time()
        times[i] = t1 - t0
    end

    println("julia\t$filename\t$(string(task))\t$(mean(times))\t$(std(times))\t$repeats")
end

"""lazily-initialized audio data"""
audio = (() -> begin
    local _filename = ""
    local _data::SampleBuf
    (filename) -> begin
        if filename != _filename
            _data = map(Float32, mono(loadwav(filename)))
            _filename = filename
        end
        _data
    end
end)()


function loadwav_fast(path)
    sfinfo = LibSndFile.SF_INFO(0, 0, 0, 0, 0, 0)
    filePtr = ccall((:sf_open, "libsndfile"), Ptr{Void}, (Ptr{UInt8}, Int32, Ptr{LibSndFile.SF_INFO}), path, LibSndFile.SFM_READ, &sfinfo)
    if filePtr == C_NULL
        errmsg = ccall((:sf_strerror, "libsndfile"), Ptr{UInt8}, (Ptr{Void},), filePtr)
        error("LibSndFile.jl error while loading $path: ", bytestring(errmsg))
    end

    source = LibSndFile.SndFileSource(UTF8String(path), filePtr, sfinfo)
    total = LibSndFile.nframes(source)
    nc = LibSndFile.nchannels(source)
    nread = 0

    buffer = Array(LibSndFile.PCM16Sample, nc, total)
    base = Base.unsafe_convert(Ptr{LibSndFile.PCM16Sample}, buffer)

    while nread < total
        n = min(4096, total - nread)
        dest = base + sizeof(Int16) * nc * nread
        nr = ccall((:sf_readf_short, "libsndfile"), Int64, (Ptr{Void}, Ptr{LibSndFile.PCM16Sample}, Int64), filePtr, dest, n)
        nread += nr
        nr == n || break
    end

    err = ccall((:sf_close, "libsndfile"), Int32, (Ptr{Void},), filePtr)
    if err != 0
        error("LibSndFile.jl error: Failed to close file")
    end

    buffer
end


loadwav(filename) = load(filename)
loadmp3(filename) = load(replace(filename, ".wav", ".mp3"))
zcr(filename) = MusicProcessing.zero_crossing_rate(audio(filename), 1024, 256)
resample(filename) = MusicProcessing.resample(audio(filename), 48000Hz)
stft(filename) = MusicProcessing.stft(audio(filename), 1024, 256)
mfcc(filename) = MusicProcessing.mfcc(audio(filename), 1024, 256; nmfcc = 20, nmels = 128)
stretch(filename) = MusicProcessing.speedup(audio(filename), 2.0)

filename = ""
target = ""
repeats = 100

try
    filename = ARGS[1]
    target = eval(Symbol(ARGS[2]))
    if length(ARGS) >= 3
        repeats = parse(Int, ARGS[3])
    end
catch
    println("usage: ./$(Base.source_path()) [filename] [benchmark name] [number of repeats; 100 by default]")
    exit(-1)
end

benchmark(filename, target, repeats)
