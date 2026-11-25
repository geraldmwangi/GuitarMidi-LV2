#include <guitarmidi/fretboard_api.hpp>
#include <guitarmidi/logging.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <cstdint>

class ConsoleLog : public LoggingAPI {
public:
    void info(std::string m, va_list a) override { std::printf("[info] "); std::vprintf(m.c_str(), a); std::printf("\n"); }
    void error(std::string m, va_list a) override { std::fprintf(stderr, "[error] "); std::vfprintf(stderr, m.c_str(), a); std::fprintf(stderr, "\n"); }
    void warn(std::string m, va_list a) override { std::printf("[warn] "); std::vprintf(m.c_str(), a); std::printf("\n"); }
};

// g_logger is defined by the core library (core/src/logging.cpp); attach the
// console logger in main() instead of defining it here.

class Out : public GuitarMidiOutput {
public:
    void initializeSequence() override {}
    void finalizeSequence() override {}
    void sendMidiMessage(uint8_t m[3], int64_t f) override {
        std::printf("midi %u %u %u @%lld\n", m[0], m[1], m[2], (long long)f);
    }
};

static int run_once(int block, int rate, const char* bundle) {
    Out* out = new Out();
    FretBoardAPI* fb = creatFretBoard(out);
    if (!fb) { std::fprintf(stderr, "creatFretBoard failed\n"); delete out; return 2; }

    if (!fb->initialize(bundle, rate, block)) {
        std::fprintf(stderr, "initialize failed (block=%d rate=%d)\n", block, rate);
        return 3;
    }

    float gain = 0.0f, expr = 0.0f, sm = 0.5f, on = 0.5f, off = 0.5f, smo = 0.5f, oe = 0.5f, ofe = 0.5f;
    fb->setGain(&gain);
    fb->setExpressivity(&expr);
    fb->setSmoothing(&sm);
    fb->setOnsetThreshold(&on);
    fb->setOffsetThreshold(&off);
    fb->setSmoothingOffset(&smo);
    fb->setOnsetEnergyThreshold(&oe);
    fb->setOffsetEnergyThreshold(&ofe);

    std::vector<float> audio(block, 0.0f);
    float phase = 0.0f;
    const float inc = 2.0f * 3.14159265f * 220.0f / (float)rate;
    for (int n = 0; n < 40; ++n) {
        for (int i = 0; i < block; ++i) {
            audio[i] = 0.3f * std::sin(phase);
            phase += inc;
        }
        fb->setAudioInput(audio.data());
        fb->process(block);
    }
    fb->finalize();
    std::printf("OK block=%d rate=%d\n", block, rate);
    std::fflush(stdout);
    delete out;
    return 0;
}

static void usage(const char* prog) {
    std::fprintf(stderr,
        "usage: %s [block] [bundle_dir] [samplerate]\n"
        "       %s -m [bundle_dir]   run rate/block matrix\n",
        prog, prog);
}

int main(int argc, char** argv) {
    g_logger.setLogger(new ConsoleLog);
    const char* bundle = "ext/GuitarMidi-LV2/python/neuralnetmodelling";
    bool matrix = false;
    std::vector<const char*> pos;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-m") == 0) matrix = true;
        else if (std::strcmp(argv[i], "-h") == 0) { usage(argv[0]); return 0; }
        else pos.push_back(argv[i]);
    }

    if (matrix) {
        if (pos.size() > 1) bundle = pos[1];
        else if (pos.size() == 1) bundle = pos[0];
        const int rates[] = {22050, 32000, 44100, 48000, 88200, 96000};
        const int blocks[] = {100, 256, 1024, 2048, 4096};
        int fails = 0;
        for (int r : rates) {
            for (int b : blocks) {
                if (run_once(b, r, bundle) != 0) ++fails;
            }
        }
        std::printf("%s matrix done (%d failures)\n", fails ? "FAIL" : "PASS", fails);
        return fails ? 1 : 0;
    }

    int block = pos.size() > 0 ? std::atoi(pos[0]) : 256;
    if (pos.size() > 1) bundle = pos[1];
    int rate = pos.size() > 2 ? std::atoi(pos[2]) : 48000;
    if (block <= 0 || rate <= 0) { usage(argv[0]); return 1; }
    return run_once(block, rate, bundle);
}
