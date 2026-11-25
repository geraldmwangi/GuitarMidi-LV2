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
 * End-to-end timing tests for the detection pipeline.
 *
 * These need the shipped model (python/neuralnetmodelling/guitarmidi.tflite)
 * and exercise the contract the timestamps introduced:
 *
 *  - notes are detected at every host block size, including sizes that do not
 *    divide the analysis hop (100, 300), because the model now always sees a
 *    contiguous frame,
 *  - every emitted event is stamped inside the block that produced it and
 *    never before the audio that caused it,
 *  - event times are monotone,
 *  - with a model slower than real time the bounded queues refuse frames
 *    (instead of overwriting them), the pipeline reports the resulting holes,
 *    and no note is left hanging.
 */
#include <guitarmidi/fretboard.hpp>

#include <algorithm>
#include <dirent.h>
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
int failures = 0;

void check(bool condition, const std::string &what, int line)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, line, what.c_str());
        ++failures;
    }
}

#define CHECK(condition) check((condition), #condition, __LINE__)
#define CHECK_MSG(condition, message) check((condition), (message), __LINE__)

constexpr int kSampleRate = 48000;

struct CaptureOutput : GuitarMidiOutput
{
    struct Event
    {
        int64_t frame;
        int64_t blockStart;
        int blockSize;
        uint8_t note;
        uint8_t velocity;
        bool on;
    };

    std::vector<Event> events;
    int64_t blockStart = 0;
    int blockSize = 0;

    void initializeSequence() override {}
    void finalizeSequence() override {}
    void sendMidiMessage(uint8_t midinote[3], int64_t frames) override
    {
        events.push_back({blockStart + frames, blockStart, blockSize,
                          midinote[1], midinote[2], midinote[2] > 0});
    }
};

double noteFrequency(int midi)
{
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

void addPluck(std::vector<float> &buffer, int start, int midi, double amplitude,
              int sampleRate = kSampleRate)
{
    const double f0 = noteFrequency(midi);
    const int length = std::min<int>(static_cast<int>(sampleRate * 0.4),
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

struct Controls
{
    float gain = 20.0f;
    float expressivity = 7.0f;
    float smoothing = 0.3f;
    float smoothingOffset = 0.3f;
    float onsetThreshold = 0.80f;
    float offsetThreshold = 0.10f;
    float onsetEnergy = -20.0f;
    float offsetEnergy = -20.0f;
};

struct RunResult
{
    std::vector<double> latenciesMs;
    int detected = 0;
    int falsePositives = 0;
    int outOfOrder = 0;
    uint32_t declaredLatencyFrames = 0;
    GuitarMidi::NoteInferenceStats stats;
};

RunResult runCase(int blockSize, const std::vector<int> &pitches, double spacing,
                  bool realtime, int inferenceDelayMs, int backpressureMs = 0,
                  int sampleRate = kSampleRate)
{
    Controls controls;
    auto *output = new CaptureOutput();
    output->blockSize = blockSize;
    FretBoard board(output);
    board.setGain(&controls.gain);
    board.setExpressivity(&controls.expressivity);
    board.setSmoothing(&controls.smoothing);
    board.setSmoothingOffset(&controls.smoothingOffset);
    board.setOnsetThreshold(&controls.onsetThreshold);
    board.setOffsetThreshold(&controls.offsetThreshold);
    board.setOnsetEnergyThreshold(&controls.onsetEnergy);
    board.setOffsetEnergyThreshold(&controls.offsetEnergy);
    board.setInferenceDelayMs(inferenceDelayMs);
    board.setBackpressureMs(backpressureMs);

    if (!board.initialize(GUITARMIDI_TEST_MODEL_DIR, sampleRate, blockSize))
    {
        std::fprintf(stderr, "FAIL: could not initialize the model from %s\n", GUITARMIDI_TEST_MODEL_DIR);
        std::exit(1);
    }
    RunResult result;
    result.declaredLatencyFrames = board.getLatencyFrames();

    const double leadIn = 0.35;
    const int totalSamples = static_cast<int>((leadIn + pitches.size() * spacing + 0.5) * sampleRate);
    std::vector<float> audio(totalSamples, 0.0f);
    std::vector<int> onsets;
    for (size_t i = 0; i < pitches.size(); ++i)
    {
        const int start = static_cast<int>((leadIn + i * spacing) * sampleRate);
        onsets.push_back(start);
        addPluck(audio, start, pitches[i], 0.25, sampleRate);
    }

    std::vector<float> block(blockSize, 0.0f);
    const int blocks = (totalSamples + blockSize - 1) / blockSize;
    const auto startTime = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b)
    {
        const int offset = b * blockSize;
        const int n = std::min(blockSize, totalSamples - offset);
        std::fill(block.begin(), block.end(), 0.0f);
        std::memcpy(block.data(), audio.data() + offset, n * sizeof(float));
        if (realtime)
        {
            const auto due = startTime +
                             std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                 std::chrono::duration<double>(static_cast<double>(offset) / sampleRate));
            std::this_thread::sleep_until(due);
        }
        output->blockStart = offset;
        board.setAudioInput(block.data());
        board.process(n);
    }

    result.stats = board.getInferenceStats();

    // Every event must be stamped inside the block that emitted it.
    for (const auto &event : output->events)
        CHECK_MSG(event.frame >= event.blockStart && event.frame - event.blockStart < event.blockSize,
                  "event outside its block");

    for (size_t e = 1; e < output->events.size(); ++e)
        if (output->events[e].frame < output->events[e - 1].frame)
            ++result.outOfOrder;

    int matched[16];
    std::fill(std::begin(matched), std::end(matched), -1);
    for (size_t i = 0; i < pitches.size(); ++i)
    {
        const int windowStart = onsets[i];
        const int windowEnd = (i + 1 < pitches.size())
                                  ? onsets[i + 1]
                                  : onsets[i] + static_cast<int>(0.45 * sampleRate);
        for (size_t e = 0; e < output->events.size(); ++e)
        {
            const auto &event = output->events[e];
            if (event.on && event.note == pitches[i] && event.frame >= windowStart && event.frame < windowEnd)
            {
                matched[i] = static_cast<int>(e);
                result.latenciesMs.push_back(
                    static_cast<double>(event.frame - windowStart) * 1000.0 / sampleRate);
                ++result.detected;
                break;
            }
        }
    }
    for (size_t e = 0; e < output->events.size(); ++e)
    {
        bool expected = false;
        for (size_t i = 0; i < pitches.size(); ++i)
            if (matched[i] == static_cast<int>(e))
                expected = true;
        if (output->events[e].on && !expected)
            ++result.falsePositives;
    }

    // No note may stay held once the input has been silent for a while.
    float smoothed[NUM_NOTES] = {};
    float energies[NUM_NOTES] = {};
    float momentary[NUM_NOTES] = {};
    float momentaryEnergy[NUM_NOTES] = {};
    bool noteOn[NUM_NOTES] = {};
    board.getNoteDebugData(smoothed, energies, momentary, momentaryEnergy, noteOn);
    for (int i = 0; i < NUM_NOTES; ++i)
        CHECK_MSG(!noteOn[i], "note still held after the input went silent");

    board.finalize();
    return result;
}

void wire(FretBoard &board, Controls &controls)
{
    board.setGain(&controls.gain);
    board.setExpressivity(&controls.expressivity);
    board.setSmoothing(&controls.smoothing);
    board.setSmoothingOffset(&controls.smoothingOffset);
    board.setOnsetThreshold(&controls.onsetThreshold);
    board.setOffsetThreshold(&controls.offsetThreshold);
    board.setOnsetEnergyThreshold(&controls.onsetEnergy);
    board.setOffsetEnergyThreshold(&controls.offsetEnergy);
}

int threadCount()
{
    int count = 0;
    DIR *dir = opendir("/proc/self/task");
    if (dir == nullptr)
        return -1;
    while (dirent *entry = readdir(dir))
        if (entry->d_name[0] != '.')
            ++count;
    closedir(dir);
    return count;
}

/* Feeds `audio` from `from` to `to` in host blocks, optionally paced in real
 * time; `beforeBlock` runs before each block with its start frame. */
template <typename Hook>
void feedBlocks(FretBoard &board, CaptureOutput *output, const std::vector<float> &audio,
                int from, int to, int blockSize, bool realtime, Hook beforeBlock)
{
    std::vector<float> block(blockSize, 0.0f);
    const auto startTime = std::chrono::steady_clock::now();
    for (int offset = from; offset < to; offset += blockSize)
    {
        const int n = std::min(blockSize, to - offset);
        std::fill(block.begin(), block.end(), 0.0f);
        std::memcpy(block.data(), audio.data() + offset, n * sizeof(float));
        if (realtime)
            std::this_thread::sleep_until(
                startTime + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                std::chrono::duration<double>(static_cast<double>(offset - from) / kSampleRate)));
        beforeBlock(offset);
        output->blockStart = offset;
        output->blockSize = blockSize;
        board.setAudioInput(block.data());
        board.process(n);
    }
}

/* A long, slowly decaying note: the case where cutting and re-triggering is
 * audible. */
void addSustainedNote(std::vector<float> &buffer, int start, int midi)
{
    const double f0 = noteFrequency(midi);
    const int length = static_cast<int>(buffer.size()) - start;
    for (int i = 0; i < length; ++i)
    {
        const double t = static_cast<double>(i) / kSampleRate;
        const double envelope = std::min(1.0, t / 0.002) * std::exp(-t / 1.5);
        double sample = 0.0;
        for (int h = 1; h <= 10; ++h)
            sample += std::sin(2.0 * M_PI * f0 * h * t) / h;
        buffer[start + i] += static_cast<float>(0.25 * envelope * sample * 0.3);
    }
}

int countEvents(const CaptureOutput &output, int note, bool on)
{
    int count = 0;
    for (const auto &event : output.events)
        if (event.note == note && event.on == on)
            ++count;
    return count;
}

double meanOf(const std::vector<double> &values)
{
    if (values.empty())
        return 0.0;
    double sum = 0.0;
    for (double value : values)
        sum += value;
    return sum / values.size();
}

double stddevOf(const std::vector<double> &values)
{
    if (values.empty())
        return 0.0;
    const double mean = meanOf(values);
    double sum = 0.0;
    for (double value : values)
        sum += (value - mean) * (value - mean);
    return std::sqrt(sum / values.size());
}

} // namespace

int main()
{
    // The declared latency must match the scheduling constants: the base
    // bound for blocks up to one hop, and a bound that grows with the host
    // block size.
    {
        GuitarMidi::NoteInferencer reference(nullptr);
        CHECK(reference.latencyFrames() == (1 + PIPELINE_LATENCY_HOPS) * BUFFER_SIZE);
        reference.setHostBufferSize(1024);
        CHECK(reference.latencyFrames() == (1 + PIPELINE_LATENCY_HOPS - 1 + 4) * BUFFER_SIZE);
        reference.setHostBufferSize(2048);
        CHECK(reference.latencyFrames() == (1 + PIPELINE_LATENCY_HOPS - 1 + 8) * BUFFER_SIZE);
    }

    const std::vector<int> pitches = {40, 45, 50};
    const double spacing = 0.7;

    for (int blockSize : {64, 100, 256, 300, 480, 1024, 2048, 4096})
    {
        const uint32_t boundHops = PIPELINE_LATENCY_HOPS - 1 +
                                   (blockSize + BUFFER_SIZE - 1) / BUFFER_SIZE;
        const double declaredLatencyMs = 1000.0 * (1 + boundHops) * BUFFER_SIZE / kSampleRate;
        const RunResult result = runCase(blockSize, pitches, spacing, true, 0);
        const double maxLatency = result.latenciesMs.empty()
                                      ? 0.0
                                      : *std::max_element(result.latenciesMs.begin(), result.latenciesMs.end());
        std::printf("block %4d: detected %d/%zu, latency mean %.2f ms, max %.2f ms, "
                    "sd %.2f ms, false positives %d, out of order %d, gaps %llu, "
                    "input drops %llu, output drops %llu, late %llu, stale %llu, "
                    "inference mean %.2f ms max %.2f ms\n",
                    blockSize, result.detected, pitches.size(),
                    meanOf(result.latenciesMs), maxLatency, stddevOf(result.latenciesMs),
                    result.falsePositives, result.outOfOrder,
                    static_cast<unsigned long long>(result.stats.streamGaps),
                    static_cast<unsigned long long>(result.stats.inputDropped),
                    static_cast<unsigned long long>(result.stats.outputDropped),
                    static_cast<unsigned long long>(result.stats.late),
                    static_cast<unsigned long long>(result.stats.staleDropped),
                    result.stats.meanInferenceMs, result.stats.maxInferenceMs);

        CHECK_MSG(result.detected == static_cast<int>(pitches.size()),
                  "note missing at block size " + std::to_string(blockSize));
        CHECK_MSG(result.outOfOrder == 0, "events out of order at block size " + std::to_string(blockSize));
        CHECK_MSG(result.falsePositives <= 4,
                  "false positives at block size " + std::to_string(blockSize));
        for (double latency : result.latenciesMs)
        {
            CHECK_MSG(latency >= 0.0, "event before its onset");
            // Declared delay plus one block of quantisation and the smoothing
            // ramp the detector needs to cross its threshold.
            CHECK_MSG(latency <= declaredLatencyMs + 60.0,
                      "latency above the declared delay plus margin at block size " +
                          std::to_string(blockSize));
        }
        if (result.stats.maxInferenceMs < 4.0)
        {
            CHECK_MSG(result.stats.inputDropped == 0,
                      "a fast model must not overflow the input queue");
            CHECK_MSG(result.stats.streamGaps == 0,
                      "a fast model must not produce stream gaps");
            CHECK_MSG(result.stats.late == 0,
                      "a fast model must not miss the schedule");
        }
    }

    // A model that cannot keep up: the queue must refuse frames and the
    // pipeline must report the holes and the late results.
    // One note is enough: the point is that a model slower than the schedule
    // is reported and flushed, not that it recognises the whole phrase.
    const std::vector<int> slowPitches = {50};
    const RunResult slow = runCase(256, slowPitches, 0.6, true, 40);
    std::printf("slow inference: detected %d/%zu, gaps %llu, input drops %llu, "
                "output drops %llu, stale %llu, late %llu\n",
                slow.detected, slowPitches.size(),
                static_cast<unsigned long long>(slow.stats.streamGaps),
                static_cast<unsigned long long>(slow.stats.inputDropped),
                static_cast<unsigned long long>(slow.stats.outputDropped),
                static_cast<unsigned long long>(slow.stats.staleDropped),
                static_cast<unsigned long long>(slow.stats.late));
    CHECK_MSG(slow.stats.inputDropped > 0, "slow inference must overflow the bounded input queue");
    CHECK_MSG(slow.stats.streamGaps > 0, "holes in the stream must be reported");
    CHECK_MSG(slow.stats.late > 0, "a model slower than the schedule must miss it");
    CHECK_MSG(slow.outOfOrder == 0, "events out of order with a slow model");

    // Offline rendering processes faster than real time. With the real-time
    // policy every frame would be refused; backpressure turns the queue into a
    // proper hand-off so a render keeps its notes. The timing must match the
    // real-time run because the scheduler does not depend on wall time.
    const RunResult offline = runCase(256, pitches, spacing, false, 0, 200);
    std::printf("offline backpressure: detected %d/%zu, hops %llu, gaps %llu, "
                "input drops %llu, note-ons %llu, inference %.2f ms, "
                "latency mean %.2f ms, max %.2f ms\n",
                offline.detected, pitches.size(),
                static_cast<unsigned long long>(offline.stats.hopsProcessed),
                static_cast<unsigned long long>(offline.stats.streamGaps),
                static_cast<unsigned long long>(offline.stats.inputDropped),
                static_cast<unsigned long long>(offline.stats.noteOns),
                offline.stats.meanInferenceMs,
                meanOf(offline.latenciesMs),
                offline.latenciesMs.empty() ? 0.0
                                            : *std::max_element(offline.latenciesMs.begin(),
                                                                offline.latenciesMs.end()));
    CHECK_MSG(offline.detected == static_cast<int>(pitches.size()),
              "offline rendering with backpressure must keep every note");
    CHECK_MSG(offline.stats.inputDropped == 0, "backpressure must not drop frames");
    CHECK_MSG(offline.outOfOrder == 0, "events out of order in an offline render");

    // Non-native sample rate: the resampled path must detect the phrase, and
    // the declared latency must be scaled from the native analysis rate into
    // host frames (706 instead of 768 at 44.1 kHz).
    {
        constexpr int kOtherRate = 44100;
        const RunResult resampled = runCase(256, pitches, spacing, false, 0, 200, kOtherRate);
        const uint32_t expected = static_cast<uint32_t>(
            (static_cast<uint64_t>((1 + PIPELINE_LATENCY_HOPS) * BUFFER_SIZE) * kOtherRate +
             NATIVE_SAMPLERATE / 2) /
            NATIVE_SAMPLERATE);
        std::printf("resampled %d Hz: detected %d/%zu, declared latency %u frames, "
                    "latency mean %.2f ms, max %.2f ms\n",
                    kOtherRate, resampled.detected, pitches.size(),
                    resampled.declaredLatencyFrames,
                    meanOf(resampled.latenciesMs),
                    resampled.latenciesMs.empty()
                        ? 0.0
                        : *std::max_element(resampled.latenciesMs.begin(),
                                            resampled.latenciesMs.end()));
        CHECK_MSG(resampled.declaredLatencyFrames == expected,
                  "the declared latency must be converted to host frames");
        CHECK_MSG(resampled.detected == static_cast<int>(pitches.size()),
                  "note missing on the resampled path");
        CHECK_MSG(resampled.outOfOrder == 0, "events out of order on the resampled path");
        for (double latency : resampled.latenciesMs)
            CHECK_MSG(latency <= 1000.0 * expected / kOtherRate + 60.0,
                      "latency above the declared delay plus margin on the resampled path");
    }

    // An LV2 deactivate/activate cycle: finalize() stops the inference
    // worker, and activate() must bring it back, or the plugin stays silent
    // for the rest of the session (hosts cycle instances around buffer-size
    // changes and graph edits).
    {
        Controls controls;
        auto *output = new CaptureOutput();
        const int blockSize = 256;
        output->blockSize = blockSize;
        FretBoard board(output);
        board.setGain(&controls.gain);
        board.setExpressivity(&controls.expressivity);
        board.setSmoothing(&controls.smoothing);
        board.setSmoothingOffset(&controls.smoothingOffset);
        board.setOnsetThreshold(&controls.onsetThreshold);
        board.setOffsetThreshold(&controls.offsetThreshold);
        board.setOnsetEnergyThreshold(&controls.onsetEnergy);
        board.setOffsetEnergyThreshold(&controls.offsetEnergy);
        board.setBackpressureMs(200); // deterministic offline processing
        CHECK_MSG(board.initialize(GUITARMIDI_TEST_MODEL_DIR, kSampleRate, 2048),
                  "initialize for the lifecycle case");

        const std::vector<int> phrase = {40, 45, 50};
        const double spacing = 0.7;
        const double leadIn = 0.35;
        const double gap = 0.6;
        const double resume = leadIn + phrase.size() * spacing + gap;
        const int totalSamples = static_cast<int>(
            (resume + phrase.size() * spacing + 0.5) * kSampleRate);
        std::vector<float> audio(totalSamples, 0.0f);
        std::vector<int> onsets[2];
        for (size_t i = 0; i < phrase.size(); ++i)
        {
            const int start = static_cast<int>((leadIn + i * spacing) * kSampleRate);
            onsets[0].push_back(start);
            addPluck(audio, start, phrase[i], 0.25);
        }
        for (size_t i = 0; i < phrase.size(); ++i)
        {
            const int start = static_cast<int>((resume + i * spacing) * kSampleRate);
            onsets[1].push_back(start);
            addPluck(audio, start, phrase[i], 0.25);
        }

        int processed = 0;
        std::vector<float> block(blockSize, 0.0f);
        auto feed = [&](int upto) {
            while (processed < upto)
            {
                const int n = std::min(blockSize, upto - processed);
                std::fill(block.begin(), block.end(), 0.0f);
                std::memcpy(block.data(), audio.data() + processed, n * sizeof(float));
                output->blockStart = processed;
                board.setAudioInput(block.data());
                board.process(n);
                processed += n;
            }
        };

        // Phrase A, then the LV2 cycle in the silent gap, then phrase B.
        feed(static_cast<int>((leadIn + phrase.size() * spacing + gap * 0.5) * kSampleRate));
        board.finalize();          // LV2 deactivate
        CHECK_MSG(board.reactivate(), "reactivate must restart the inference worker");
        feed(totalSamples);
        board.finalize();

        int detected[2] = {0, 0};
        for (int half = 0; half < 2; ++half)
            for (size_t i = 0; i < phrase.size(); ++i)
                for (const auto &event : output->events)
                    if (event.on && event.note == phrase[i] &&
                        event.frame >= onsets[half][i] &&
                        event.frame < onsets[half][i] + static_cast<int>(0.45 * kSampleRate))
                    {
                        ++detected[half];
                        break;
                    }
        std::printf("lifecycle: detected %d/%zu before deactivate, %d/%zu after reactivate\n",
                    detected[0], phrase.size(), detected[1], phrase.size());
        CHECK_MSG(detected[0] == static_cast<int>(phrase.size()),
                  "notes missing before the deactivate/activate cycle");
        CHECK_MSG(detected[1] == static_cast<int>(phrase.size()),
                  "notes missing after the deactivate/activate cycle");
    }

    // A model that does not load must be reported, not terminate the host.
    {
        Controls controls;
        FretBoard board(new CaptureOutput());
        wire(board, controls);
        static const char junk[64] = "not a TensorFlow Lite flatbuffer";
        CHECK_MSG(!board.initializeFromBuffer(junk, sizeof junk, kSampleRate, 256),
                  "a broken model must make initialization fail");
        CHECK_MSG(!board.reactivate(), "reactivate must fail cleanly without a model");
    }

    // Deactivate/activate must not reload the model: hosts cycle instances
    // for latency queries (Ardour does it for every plugin with a latency
    // port), and each reload used to leak the XNNPack delegate and its
    // thread pool.
    {
        Controls controls;
        FretBoard board(new CaptureOutput());
        wire(board, controls);
        CHECK(board.initialize(GUITARMIDI_TEST_MODEL_DIR, kSampleRate, 256));
        const int before = threadCount();
        for (int cycle = 0; cycle < 10; ++cycle)
        {
            board.finalize();
            CHECK(board.reactivate());
        }
        // A joined thread can linger in /proc for a moment after join()
        // returns; give it time to go before counting. A leak (the old
        // behaviour: three pool threads per cycle) does not go away.
        int after = threadCount();
        for (int i = 0; i < 100 && after > before; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            after = threadCount();
        }
        std::printf("lifecycle threads: %d before, %d after 10 cycles\n", before, after);
        CHECK_MSG(after == before, "deactivate/activate cycles must not leak threads");
        board.finalize();
    }

    // A note held at deactivate is released at the next process() call:
    // deactivate cannot write MIDI, and the synth downstream must not hang.
    {
        Controls controls;
        auto *output = new CaptureOutput();
        FretBoard board(output);
        wire(board, controls);
        board.setBackpressureMs(200);
        CHECK(board.initialize(GUITARMIDI_TEST_MODEL_DIR, kSampleRate, 256));
        std::vector<float> audio(2 * kSampleRate, 0.0f);
        addSustainedNote(audio, kSampleRate / 4, 45);
        feedBlocks(board, output, audio, 0, kSampleRate, 256, false, [](int) {});
        CHECK_MSG(countEvents(*output, 45, true) == 1, "the sustained note must be on before deactivate");
        CHECK_MSG(countEvents(*output, 45, false) == 0, "the sustained note must still be held");
        board.finalize();
        CHECK(board.reactivate());
        const size_t mark = output->events.size();
        std::vector<float> silence(256, 0.0f);
        feedBlocks(board, output, silence, 0, 256, 256, false, [](int) {});
        bool released = false;
        for (size_t e = mark; e < output->events.size(); ++e)
            if (output->events[e].note == 45 && !output->events[e].on)
                released = output->events[e].frame == output->events[e].blockStart;
        CHECK_MSG(released, "a note held at deactivate must be released at the start of the next block");
        board.finalize();
    }

    // One worker stall (a preemption) while a note sustains: the late results
    // are counted, but the note must neither be cut nor re-triggered.
    {
        Controls controls;
        auto *output = new CaptureOutput();
        FretBoard board(output);
        wire(board, controls);
        const int blockSize = 128;
        CHECK(board.initialize(GUITARMIDI_TEST_MODEL_DIR, kSampleRate, blockSize));
        std::vector<float> audio(3 * kSampleRate, 0.0f);
        addSustainedNote(audio, kSampleRate / 4, 45);
        const int stallAt = kSampleRate * 5 / 4;
        bool stalled = false;
        feedBlocks(board, output, audio, 0, static_cast<int>(audio.size()), blockSize, true,
                   [&](int offset) {
                       if (!stalled && offset >= stallAt)
                       {
                           board.setInferenceDelayMs(15);
                           stalled = true;
                       }
                       else if (stalled && offset >= stallAt + 2 * BUFFER_SIZE)
                           board.setInferenceDelayMs(0);
                   });
        const auto stats = board.getInferenceStats();
        std::printf("worker stall: note-ons %d, late %llu, flushes %llu\n",
                    countEvents(*output, 45, true),
                    static_cast<unsigned long long>(stats.late),
                    static_cast<unsigned long long>(stats.flushes));
        CHECK_MSG(stats.late > 0, "the stall must make results miss the schedule");
        CHECK_MSG(countEvents(*output, 45, true) == 1,
                  "a worker stall must not cut and re-trigger a sustained note");
        CHECK_MSG(stats.flushes == 0, "a short stall must not flush the held notes");
        board.finalize();
    }

    if (failures == 0)
        std::printf("test_pipeline_timing: OK\n");
    else
        std::printf("test_pipeline_timing: %d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
