/* GuitarMidi-LV2 Library
 * Copyright (C) 2022 Gerald Mwangi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */
/*
 * Offline timing report for the GuitarMidi detection pipeline.
 *
 * Feeds synthetic plucks at known sample positions through the plugin's
 * public FretBoardAPI and reports, per note, the delay between the true onset
 * and the MIDI note-on, plus jitter and false positives. It deliberately uses
 * only the interface that existed before the timestamps/latency work, so the
 * same binary can be built against the old and the new implementation and the
 * numbers are directly comparable.
 *
 * Usage:
 *   latency_report --model <dir containing guitarmidi.tflite>
 *                  [--block N] [--realtime 0|1] [--notes N] [--sr N]
 *                  [--gain dB] [--csv file]
 */
#include <guitarmidi/fretboard_api.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace
{

struct CaptureOutput : GuitarMidiOutput
{
    struct Event
    {
        int64_t frame;
        uint8_t note;
        uint8_t velocity;
        bool on;
    };

    std::vector<Event> events;
    int64_t blockStart = 0;

    void initializeSequence() override {}
    void finalizeSequence() override {}
    void sendMidiMessage(uint8_t midinote[3], int64_t frames) override
    {
        events.push_back({blockStart + frames, midinote[1], midinote[2], midinote[2] > 0});
    }
};

struct Options
{
    std::string model;
    std::string csv;
    int block = 256;
    int sampleRate = 48000;
    int notes = 6;
    int pitch = -1; // >= 0: repeat this pitch (measures jitter, not pitch spread)
    bool realtime = true;
    int backpressureMs = 0;
    double gainDb = 20.0;
};

double noteFrequency(int midi)
{
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

/* A decaying harmonic stack with a short attack: enough for a detector trained
 * on guitar to see a note, without depending on a sample library. */
void addPluck(std::vector<float> &buffer, int start, int midi, int sampleRate, double amplitude)
{
    const double f0 = noteFrequency(midi);
    const int length = std::min<int>(static_cast<int>(sampleRate * 0.45),
                                     static_cast<int>(buffer.size()) - start);
    for (int i = 0; i < length; ++i)
    {
        const double t = static_cast<double>(i) / sampleRate;
        const double attack = std::min(1.0, t / 0.002);
        const double envelope = attack * std::exp(-t / 0.18);
        double sample = 0.0;
        for (int h = 1; h <= 10; ++h)
        {
            const double frequency = f0 * h;
            if (frequency > sampleRate * 0.45)
                break;
            sample += std::sin(2.0 * M_PI * frequency * t) / h;
        }
        buffer[start + i] += static_cast<float>(amplitude * envelope * sample * 0.3);
    }
}

Options parseArgs(int argc, char **argv)
{
    Options options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        auto next = [&]() -> const char * { return (i + 1 < argc) ? argv[++i] : ""; };
        if (arg == "--model")
            options.model = next();
        else if (arg == "--block")
            options.block = std::atoi(next());
        else if (arg == "--sr")
            options.sampleRate = std::atoi(next());
        else if (arg == "--notes")
            options.notes = std::atoi(next());
        else if (arg == "--pitch")
            options.pitch = std::atoi(next());
        else if (arg == "--backpressure")
            options.backpressureMs = std::atoi(next());
        else if (arg == "--realtime")
            options.realtime = std::atoi(next()) != 0;
        else if (arg == "--gain")
            options.gainDb = std::atof(next());
        else if (arg == "--csv")
            options.csv = next();
        else
        {
            std::fprintf(stderr, "unknown argument %s\n", arg.c_str());
            std::exit(2);
        }
    }
    if (options.model.empty())
    {
        std::fprintf(stderr, "--model <dir with guitarmidi.tflite> is required\n");
        std::exit(2);
    }
    return options;
}

} // namespace

int main(int argc, char **argv)
{
    const Options options = parseArgs(argc, argv);

    // Open strings and first positions: inside the model's 37-note range.
    const int scale[] = {40, 45, 50, 55, 59, 64, 43, 48};
    std::vector<int> pitches;
    for (int i = 0; i < std::min<int>(options.notes, 64); ++i)
        pitches.push_back(options.pitch >= 0 ? options.pitch : scale[i % 8]);
    const int noteCount = static_cast<int>(pitches.size());

    const double noteSpacing = 0.75; // seconds between plucks
    const int totalSamples = static_cast<int>((noteCount + 1) * noteSpacing * options.sampleRate);

    std::vector<float> audio(totalSamples, 0.0f);
    std::vector<int> onsets;
    for (int i = 0; i < noteCount; ++i)
    {
        const int start = static_cast<int>((0.3 + i * noteSpacing) * options.sampleRate);
        onsets.push_back(start);
        addPluck(audio, start, pitches[i], options.sampleRate, 0.25);
    }

    // Control values matching the plugin's documented defaults.
    float gain = static_cast<float>(options.gainDb);
    float expressivity = 7.0f;
    float smoothing = 0.3f;
    float smoothingOffset = 0.3f;
    float onsetThreshold = 0.80f;
    float offsetThreshold = 0.10f;
    float onsetEnergy = -20.0f;
    float offsetEnergy = -20.0f;

    // creatFretBoard takes ownership of the MIDI output (the note inferencer
    // deletes it), so it must live on the heap.
    auto *capture = new CaptureOutput();
    FretBoardAPI *fretboard = creatFretBoard(capture);
    fretboard->setGain(&gain);
    fretboard->setExpressivity(&expressivity);
    fretboard->setSmoothing(&smoothing);
    fretboard->setSmoothingOffset(&smoothingOffset);
    fretboard->setOnsetThreshold(&onsetThreshold);
    fretboard->setOffsetThreshold(&offsetThreshold);
    fretboard->setOnsetEnergyThreshold(&onsetEnergy);
    fretboard->setOffsetEnergyThreshold(&offsetEnergy);
    // Offline renders (realtime 0) need this to keep every note; see the
    // backpressure note in docs/inference-timing.md.
    fretboard->setBackpressureMs(options.backpressureMs);

    if (!fretboard->initialize(options.model, options.sampleRate, options.block))
    {
        std::fprintf(stderr, "initialization failed (model: %s)\n", options.model.c_str());
        delete fretboard;
        return 1;
    }

    std::vector<float> block(options.block, 0.0f);
    const int blocks = (totalSamples + options.block - 1) / options.block;

    const auto startTime = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b)
    {
        const int offset = b * options.block;
        const int n = std::min(options.block, totalSamples - offset);
        std::fill(block.begin(), block.end(), 0.0f);
        std::memcpy(block.data(), audio.data() + offset, n * sizeof(float));

        if (options.realtime)
        {
            const auto due = startTime + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                            std::chrono::duration<double>(static_cast<double>(offset) / options.sampleRate));
            std::this_thread::sleep_until(due);
        }

        capture->blockStart = offset;
        fretboard->setAudioInput(block.data());
        fretboard->process(n);
    }

    const double elapsed = std::chrono::duration<double>(
                               std::chrono::steady_clock::now() - startTime)
                               .count();
    fretboard->finalize();
    const std::vector<CaptureOutput::Event> events = capture->events;
    delete fretboard; // also deletes the capture object

    // Match the first note-on for each expected pitch after its onset.
    std::vector<double> latenciesMs;
    int detected = 0;
    std::vector<int> matchedEvent(noteCount, -1);
    for (int i = 0; i < noteCount; ++i)
    {
        const int windowStart = onsets[i];
        const int windowEnd = (i + 1 < noteCount)
                                  ? onsets[i + 1]
                                  : onsets[i] + static_cast<int>(0.6 * options.sampleRate);
        for (size_t e = 0; e < events.size(); ++e)
        {
            const auto &event = events[e];
            if (event.on && event.note == pitches[i] && event.frame >= windowStart && event.frame < windowEnd)
            {
                matchedEvent[i] = static_cast<int>(e);
                const double latency = static_cast<double>(event.frame - windowStart) * 1000.0 / options.sampleRate;
                latenciesMs.push_back(latency);
                ++detected;
                break;
            }
        }
    }

    int falsePositives = 0;
    int outOfOrder = 0;
    int64_t previousFrame = -1;
    for (size_t e = 0; e < events.size(); ++e)
    {
        const auto &event = events[e];
        if (event.frame < previousFrame)
            ++outOfOrder;
        previousFrame = event.frame;

        bool expected = false;
        for (int i = 0; i < noteCount; ++i)
            if (matchedEvent[i] == static_cast<int>(e))
                expected = true;
        if (event.on && !expected)
            ++falsePositives;
    }

    double mean = 0.0;
    double max = 0.0;
    double stddev = 0.0;
    for (double value : latenciesMs)
    {
        mean += value;
        max = std::max(max, value);
    }
    if (!latenciesMs.empty())
    {
        mean /= static_cast<double>(latenciesMs.size());
        for (double value : latenciesMs)
            stddev += (value - mean) * (value - mean);
        stddev = std::sqrt(stddev / static_cast<double>(latenciesMs.size()));
    }

    FILE *csv = options.csv.empty() ? nullptr : std::fopen(options.csv.c_str(), "w");
    if (csv)
        std::fprintf(csv, "block,pitch,onset,event,latency_ms\n");
    for (int i = 0; i < noteCount; ++i)
    {
        const int event = matchedEvent[i];
        const double latency = event >= 0
                                   ? static_cast<double>(events[event].frame - onsets[i]) * 1000.0 / options.sampleRate
                                   : -1.0;
        if (csv)
            std::fprintf(csv, "%d,%d,%d,%lld,%.3f\n", options.block, pitches[i], onsets[i],
                         event >= 0 ? static_cast<long long>(events[event].frame) : -1LL, latency);
    }
    if (csv)
        std::fclose(csv);

    std::printf("block=%d realtime=%d notes=%d detected=%d/%d false_positives=%d "
                "out_of_order=%d latency_mean_ms=%.2f latency_max_ms=%.2f latency_sd_ms=%.2f "
                "wall_s=%.2f\n",
                options.block, options.realtime ? 1 : 0, noteCount, detected, noteCount,
                falsePositives, outOfOrder, mean, max, stddev, elapsed);

    for (int i = 0; i < noteCount; ++i)
    {
        const int event = matchedEvent[i];
        std::printf("  note=%d onset=%d event=%lld latency_ms=%s%.2f\n", pitches[i], onsets[i],
                    event >= 0 ? static_cast<long long>(events[event].frame) : -1LL,
                    event >= 0 ? "" : "n/a ",
                    event >= 0
                        ? static_cast<double>(events[event].frame - onsets[i]) * 1000.0 / options.sampleRate
                        : -1.0);
    }

    return 0;
}
