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
 * Unit tests for the stamped SPSC ring buffer used between the audio thread
 * and the inference thread.
 *
 * The properties under test are the ones the old queue violated:
 *  - a full queue refuses the frame instead of overwriting a slot,
 *  - a frame is never torn (all elements of one frame belong together),
 *  - frames and their stamps come out in order,
 *  - the drop counter reports refusals.
 */
#include <guitarmidi/ringbuffer.hpp>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>

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

constexpr size_t kSlots = 8;
constexpr size_t kStride = 4;
using Ring = GuitarMidi::SpscRingBuffer<kStride>;

void testOrderingAndCapacity()
{
    Ring ring(kSlots);
    CHECK(ring.empty());
    CHECK(ring.size() == 0);
    CHECK(ring.capacity() == kSlots);

    float frame[kStride] = {};
    for (size_t i = 0; i < kSlots; ++i)
    {
        for (size_t j = 0; j < kStride; ++j)
            frame[j] = static_cast<float>(i * 10 + j);
        CHECK(ring.push(frame, i + 1));
    }
    CHECK(ring.size() == kSlots);

    // The queue is full: the producer must refuse, not overwrite. This is the
    // difference between a dropped note and a frame spliced from two moments.
    float overflow[kStride] = {999, 999, 999, 999};
    CHECK(!ring.push(overflow, 999));
    // Refusal is silent; the pipeline counts it when it gives up.
    CHECK(ring.dropped() == 0);
    ring.countDrop();
    CHECK(ring.dropped() == 1);
    CHECK(ring.size() == kSlots);

    float out[kStride] = {};
    uint64_t stamp = 0;
    for (size_t i = 0; i < kSlots; ++i)
    {
        CHECK(ring.pop(out, stamp));
        CHECK(stamp == i + 1);
        for (size_t j = 0; j < kStride; ++j)
            CHECK(out[j] == static_cast<float>(i * 10 + j));
    }
    CHECK(ring.empty());
    CHECK(!ring.pop(out, stamp));
}

void testReset()
{
    Ring ring(kSlots);
    float frame[kStride] = {1, 1, 1, 1};
    CHECK(ring.push(frame, 1));
    CHECK(ring.push(frame, 2));
    ring.reset();
    CHECK(ring.empty());
    CHECK(ring.dropped() == 0);
    CHECK(ring.push(frame, 7));
    float out[kStride];
    uint64_t stamp = 0;
    CHECK(ring.pop(out, stamp));
    CHECK(stamp == 7);
}

void testResize()
{
    // The capacity follows the host block size, rounded up to a power of two.
    Ring ring(5);
    CHECK(ring.capacity() == 8);
    float frame[kStride] = {1, 2, 3, 4};
    CHECK(ring.push(frame, 1));
    ring.resize(17);
    CHECK(ring.capacity() == 32);
    CHECK(ring.empty());
    for (size_t i = 0; i < 32; ++i)
        CHECK(ring.push(frame, i + 1));
    CHECK(!ring.push(frame, 99));
    ring.resize(1);
    CHECK(ring.capacity() == 1);
    CHECK(ring.push(frame, 1));
    CHECK(!ring.push(frame, 2));
}

void testNoTornFramesUnderContention()
{
    constexpr int kFrames = 200000;
    constexpr size_t kStressStride = 16;
    GuitarMidi::SpscRingBuffer<kStressStride> ring(64);

    std::atomic<int> producer_failures{0};
    std::atomic<bool> producer_done{false};
    std::atomic<bool> consumer_saw_torn{false};
    std::atomic<int> consumer_errors{0};
    std::atomic<int> consumer_frames{0};

    std::thread producer([&]() {
        float frame[kStressStride];
        for (int i = 0; i < kFrames; ++i)
        {
            const float value = static_cast<float>(i);
            for (size_t j = 0; j < kStressStride; ++j)
                frame[j] = value;
            if (!ring.push(frame, static_cast<uint64_t>(i)))
                ++producer_failures;
        }
        producer_done.store(true);
    });

    std::thread consumer([&]() {
        float frame[kStressStride];
        uint64_t stamp = 0;
        uint64_t last = 0;
        bool have_last = false;
        while (true)
        {
            if (!ring.pop(frame, stamp))
            {
                if (producer_done.load())
                    break;
                std::this_thread::yield();
                continue;
            }
            // Every element of a frame must belong to the same push.
            for (size_t j = 1; j < kStressStride; ++j)
                if (frame[j] != frame[0])
                    consumer_saw_torn = true;
            ++consumer_frames;
            if (frame[0] != static_cast<float>(stamp))
                ++consumer_errors;
            // Stamps must be strictly increasing.
            if (have_last && stamp <= last)
                ++consumer_errors;
            last = stamp;
            have_last = true;
        }
    });

    producer.join();
    consumer.join();

    CHECK(!consumer_saw_torn.load());
    CHECK(consumer_errors.load() == 0);
    // Throughput is scheduler dependent (the producer is allowed to lose the
    // race); what matters is that whatever got through was intact.
    CHECK(consumer_frames.load() > 1000);
    CHECK(producer_failures.load() <= kFrames);
}

} // namespace

int main()
{
    testOrderingAndCapacity();
    testReset();
    testResize();
    testNoTornFramesUnderContention();

    if (failures == 0)
        std::printf("test_ringbuffer: OK\n");
    else
        std::printf("test_ringbuffer: %d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
