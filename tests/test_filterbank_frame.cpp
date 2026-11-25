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
 * Reads the analysis frame the way the model sees it.
 *
 * Regression test for the frame-assembly contract:
 *  - a captured frame is a contiguous, oldest-first window of exactly
 *    BUFFER_SIZE samples per filter, never a wrapped/rotated buffer,
 *  - the frame does not depend on how the caller chunked the audio, as long
 *    as chunks respect the hop boundary (which FretBoard guarantees).
 *
 * Before this contract existed the rolling filter buffer was handed to the
 * model directly, so for host block sizes that do not divide BUFFER_SIZE the
 * model saw a window with a discontinuity in the middle, and a chunk crossing
 * the row boundary wrote into the neighbouring filter's row.
 */
#include <guitarmidi/filterbank.hpp>
#include <guitarmidi/fretboardrepresentation.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
int failures = 0;

void check(bool condition, const char *expression, int line)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, line, expression);
        ++failures;
    }
}

#define CHECK(condition) check((condition), #condition, __LINE__)

std::vector<float> makeSignal(int samples)
{
    std::vector<float> signal(samples);
    for (int i = 0; i < samples; ++i)
    {
        const double t = static_cast<double>(i) / 48000.0;
        const double burst = std::exp(-std::fmod(t, 0.4) / 0.15);
        signal[i] = static_cast<float>(
            0.3 * burst * (std::sin(2.0 * M_PI * 196.0 * t) +
                           0.5 * std::sin(2.0 * M_PI * 392.0 * t)));
    }
    return signal;
}

std::vector<float> copyFrame(const GuitarMidi::FilterBank &bank)
{
    const GuitarMidi::AudioBuffer2D frame = bank.get_frame_buffer();
    const int count = frame.num_filters * frame.window_size;
    return std::vector<float>(frame.audio_buffer_2D, frame.audio_buffer_2D + count);
}

} // namespace

int main()
{
    constexpr int kTotal = 48000;
    constexpr int kFrame = BUFFER_SIZE;
    const int samples = kTotal;
    const std::vector<float> signal = makeSignal(samples);

    GuitarMidi::FretBoardRepresentation representation;
    const auto filters = representation.get_filterrepresentations();
    // process() reads the gain control unconditionally, as the host always
    // supplies it in the plugin.
    float gainDb = 0.0f;

    // Reference pass: one hop per call.
    GuitarMidi::FilterBank aligned;
    aligned.setup(filters, 48000, kFrame);
    aligned.setGain(&gainDb);
    aligned.setInput(signal.data());
    std::vector<std::vector<float>> reference;
    for (int offset = 0; offset + kFrame <= samples; offset += kFrame)
    {
        aligned.process(kFrame, offset);
        aligned.captureFrame();
        reference.push_back(copyFrame(aligned));
    }

    // Candidate pass: irregular chunk sizes, clamped to the hop boundary the
    // way FretBoard drives the filter bank.
    GuitarMidi::FilterBank chopped;
    chopped.setup(filters, 48000, kFrame);
    chopped.setGain(&gainDb);
    chopped.setInput(signal.data());
    const int chunkPattern[] = {1, 7, 33, 97, 211, 64, 128};
    int patternIndex = 0;
    int offset = 0;
    size_t hop = 0;
    while (offset < samples)
    {
        const int toHop = kFrame - (offset % kFrame);
        const int wanted = chunkPattern[patternIndex++ % 7];
        const int n = std::min(wanted, std::min(toHop, samples - offset));
        chopped.process(n, offset);
        offset += n;
        if (offset % kFrame == 0 && hop < reference.size())
        {
            chopped.captureFrame();
            const std::vector<float> frame = copyFrame(chopped);
            CHECK(frame == reference[hop]);
            ++hop;
        }
    }
    CHECK(hop == reference.size());

    // The frame must be the newest window: a signal that stops must decay out
    // of the frame rather than leaving stale samples at the wrong end.
    GuitarMidi::FilterBank silent;
    silent.setup(filters, 48000, kFrame);
    silent.setGain(&gainDb);
    std::vector<float> quiet(samples, 0.0f);
    silent.setInput(quiet.data());
    for (int off = 0; off + kFrame <= samples; off += kFrame)
        silent.process(kFrame, off);
    silent.captureFrame();
    const std::vector<float> zeroFrame = copyFrame(silent);
    for (float value : zeroFrame)
        CHECK(value == 0.0f);

    if (failures == 0)
        std::printf("test_filterbank_frame: OK (%zu hops compared)\n", reference.size());
    else
        std::printf("test_filterbank_frame: %d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
