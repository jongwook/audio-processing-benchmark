#include <essentia/essentia.h>
#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>
#include <sys/time.h>

using namespace std;
using namespace essentia;
using namespace essentia::standard;

typedef void (*TASK)();

#define N_FRAMES(length, framesize, hopsize) (((length) - (framesize) / (hopsize)) + 1)

static double now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1.0 + tv.tv_usec / 1e6;
}

static double benchmark(TASK task, int repeats) {
    task();
    double t0 = now();
    for (int i = 0; i < repeats; i++) {
        task();
        cout << i << endl;
    }
    double t1 = now();
    return (t1 - t0) / repeats;
}

static string wavfile = "data/Phoenix_ScotchMorris_MIX.wav";
static string mp3file = "data/Phoenix_ScotchMorris_MIX.mp3";
int framesize = 1024;
int hopsize = 256;
int nbins = (framesize >> 1) + 1;

static vector<Real> buffer;

vector<Real> loadaudio(const string &path) {
    vector<Real> buffer;

    Algorithm *audio = AlgorithmFactory::instance().create("MonoLoader", "filename", path);
    audio->output("audio").set(buffer);
    audio->compute();
    delete audio;

    return buffer;
}

vector<Real>& audio() {
    if (buffer.size() == 0) {
        buffer = loadaudio(wavfile);
    }
    return buffer;
}

void loadwav() {
    loadaudio(wavfile);
}

void loadmp3() {
    loadaudio(mp3file);
}

void zcr() {
    vector<Real>& buffer = audio();
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

void resample() {
    vector<Real>& buffer = audio();
    vector<Real> resampled;

    Algorithm *resample = AlgorithmFactory::instance().create("Resample", "inputSampleRate", 44100, "outputSampleRate", 48000);
    resample->input("signal").set(buffer);
    resample->output("signal").set(resampled);
    resample->compute();

    delete resample;
}

void stft() {
    vector<Real>& buffer = audio();
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

void mfcc() {
    vector<Real>& buffer = audio();
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
    if (argc != 2) {
        cerr << "Usage " << argv[0] << " [benchmark name] [number of repeats; 100 by default]" << endl;
        return -1;
    }
    if (argc == 3) {
        repeats = atoi(argv[2]);
    }

    const char* task = argv[1];
    map<string, TASK> tasks;
    tasks["loadwav"] = loadwav;
    tasks["loadmp3"] = loadmp3;
    tasks["zcr"] = zcr;
    tasks["resample"] = resample;
    tasks["stft"] = stft;
    tasks["mfcc"] = mfcc;

    init();

    if (tasks.count(task) != 0) {
        double average = benchmark(tasks[task], repeats);
        cout << "Essentia-C++\t" << task << "\t" << average << "\t" << repeats << endl;
    } else {
        cerr << "Task" << task << " not found" << endl;
    }

    return 0;
}
