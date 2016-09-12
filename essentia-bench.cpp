#include <essentia/essentia.h>
#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>
#include <sys/time.h>
#include <math.h>

using namespace std;
using namespace essentia;
using namespace essentia::standard;

typedef void (*TASK)(const string&);

#define N_FRAMES(length, framesize, hopsize) (((length) - (framesize) / (hopsize)) + 1)

static double now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1.0 + tv.tv_usec / 1e6;
}

static void benchmark(const string& filename, TASK task, int repeats) {
    task(filename);
    double sumdt = 0.0;
    double sumdtdt = 0.0;
    for (int i = 0; i < repeats; i++) {
        cout << i << endl;
        double t0 = now();
        task(filename);
        double t1 = now();
        double dt = t1 - t0;
        sumdt += dt;
        sumdtdt += dt * dt;
    }

    double mean = sumdt / repeats;
    double stdev = sqrt( (sumdtdt / repeats - mean * mean) * repeats / (repeats - 1) );
    cout << "Essentia-C++\t" << filename << "\t" << task << "\t" << mean << "\t" << stdev << "\t" << repeats << endl;
}

static string _filename = "";

int framesize = 1024;
int hopsize = 256;
int nbins = (framesize >> 1) + 1;

static vector<Real> buffer;

vector<Real> loadaudio(const string &filename) {
    vector<Real> buffer;

    Algorithm *audio = AlgorithmFactory::instance().create("MonoLoader", "filename", filename);
    audio->output("audio").set(buffer);
    audio->compute();
    delete audio;

    return buffer;
}

vector<Real>& audio(const string &filename) {
    if (filename != _filename) {
        buffer = loadaudio(filename);
    }
    return buffer;
}

void loadwav(const string &filename) {
    loadaudio(filename);
}

void loadmp3(const string &filename) {
    string mp3name = filename;
    int index = mp3name.rfind(".wav");
    mp3name.replace(index, 4, ".mp3");
    loadaudio(mp3name);
}

void zcr(const string &filename) {
    vector<Real>& buffer = audio(filename);
    vector<Real> frame;
    Real rate;

    AlgorithmFactory& factory = AlgorithmFactory::instance();
    Algorithm *fc = factory.create("FrameCutter", "frameSize", framesize, "hopSize", hopsize);
    Algorithm *zcr = factory.create("ZeroCrossingRate");

    fc->input("signal").set(buffer);
    fc->output("frame").set(frame);
    zcr->input("signal").set(frame);
    zcr->output("zeroCrossingRate").set(rate);

    vector<Real> rates(N_FRAMES(buffer.size(), framesize, hopsize) + 10);

    for (int i = 0; i < rates.size(); i++) {
        fc->compute();
        if (!frame.size()) break;
        zcr->compute();
        rates[i] = rate;
    }

    delete fc, zcr;
}

void resample(const string &filename) {
    vector<Real>& buffer = audio(filename);
    vector<Real> resampled;

    Algorithm *resample = AlgorithmFactory::instance().create("Resample", "inputSampleRate", 44100, "outputSampleRate", 48000);
    resample->input("signal").set(buffer);
    resample->output("signal").set(resampled);
    resample->compute();

    delete resample;
}

void stft(const string &filename) {
    vector<Real>& buffer = audio(filename);
    vector<Real> frame;
    vector<Real> windowed;
    vector<complex<Real>> spectrum;

    AlgorithmFactory& factory = AlgorithmFactory::instance();
    Algorithm *fc = factory.create("FrameCutter", "frameSize", framesize, "hopSize", hopsize);
    Algorithm *w = factory.create("Windowing", "size", framesize, "type", "hann");
    Algorithm *fft = factory.create("FFT", "size", framesize);

    fc->input("signal").set(buffer);
    fc->output("frame").set(frame);
    w->input("frame").set(frame);
    w->output("frame").set(windowed);
    fft->input("frame").set(windowed);
    fft->output("fft").set(spectrum);

    vector<vector<complex<Real>>> spectrogram(N_FRAMES(buffer.size(), framesize, hopsize) + 10);

    for (int i = 0; i < spectrogram.size(); i++) {
        fc->compute();
        if (!frame.size()) break;
        w->compute();
        fft->compute();
        spectrogram[i] = spectrum;
    }

    delete fc, w, fft;
}

void mfcc(const string &filename) {
    vector<Real>& buffer = audio(filename);
    vector<Real> frame;
    vector<Real> windowed;
    vector<Real> spectrum;
    vector<Real> bands;
    vector<Real> mfcc_vector;

    AlgorithmFactory& factory = AlgorithmFactory::instance();
    Algorithm *fc = factory.create("FrameCutter", "frameSize", framesize, "hopSize", hopsize);
    Algorithm *w = factory.create("Windowing", "size", framesize, "type", "hann");
    Algorithm *spec = factory.create("Spectrum", "size", framesize);
    Algorithm *mfcc = factory.create("MFCC", "highFrequencyBound", 22050, "inputSize", nbins, "numberBands", 128, "numberCoefficients", 20);

    fc->input("signal").set(buffer);
    fc->output("frame").set(frame);
    w->input("frame").set(frame);
    w->output("frame").set(windowed);
    spec->input("frame").set(windowed);
    spec->output("spectrum").set(spectrum);
    mfcc->input("spectrum").set(spectrum);
    mfcc->output("mfcc").set(mfcc_vector);
    mfcc->output("bands").set(bands);

    vector<vector<Real>> mfcc_matrix(N_FRAMES(buffer.size(), framesize, hopsize) + 10);

    for (int i = 0; i < mfcc_matrix.size(); i++) {
        fc->compute();
        if (!frame.size()) break;
        w->compute();
        spec->compute();
        mfcc->compute();
        mfcc_matrix[i] = mfcc_vector;
    }

    delete fc, w, spec, mfcc;
}

int main(int argc, char* argv[]) {
    int repeats = 100;
    if (argc < 3) {
        cerr << "Usage " << argv[0] << " [filename] [benchmark name] [number of repeats; 100 by default]" << endl;
        return -1;
    }
    if (argc == 4) {
        repeats = atoi(argv[3]);
    }

    const char* filename = argv[1];
    const char* task = argv[2];

    map<string, TASK> tasks;
    tasks["loadwav"] = loadwav;
    tasks["loadmp3"] = loadmp3;
    tasks["zcr"] = zcr;
    tasks["resample"] = resample;
    tasks["stft"] = stft;
    tasks["mfcc"] = mfcc;

    init();

    if (tasks.count(task) != 0) {
        benchmark(filename, tasks[task], repeats);
    } else {
        cerr << "Task" << task << " not found" << endl;
    }

    return 0;
}
