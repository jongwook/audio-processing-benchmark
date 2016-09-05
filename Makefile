all: essentia-bench

essentia-bench: essentia-bench.cpp
	g++ -O3 --std=gnu++11 essentia-bench.cpp -o essentia-bench -lessentia -lfftw3 -lyaml -lavcodec -lavformat -lavutil -lsamplerate -ltag -lfftw3f

clean:
	rm essentia-bench
