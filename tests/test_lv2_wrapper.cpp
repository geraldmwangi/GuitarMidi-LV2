/* GuitarMidi-LV2 Library
 * Copyright (C) 2022 Gerald Mwangi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * See the GNU Lesser General Public License for more details.
 */
/*
 * Minimal LV2 host around the plugin's own shared library.
 *
 * The pipeline tests drive FretBoardAPI directly; this one drives the LV2
 * wrapper the way a real host does: urid:map and options features, port
 * connection by index, MIDI events read back out of the atom port, the
 * latency and free-wheeling control ports, and deactivate/activate through
 * the descriptor. It runs the phrase at host block sizes from 1 sample
 * upwards, which is below anything the pipeline tests cover.
 */
#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/urid/urid.h>
#include <lv2/options/options.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/midi/midi.h>
#include <guitarmidi/common.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
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
constexpr uint32_t kMidiCapacity = 1u << 14;
constexpr uint32_t kDebugCapacity = 1u << 16;

/* A URID map that assigns small integers in first-seen order; enough for a
 * single-instance test host. */
struct Urmap
{
    std::map<std::string, LV2_URID> ids;
    LV2_URID_Map map{this, map_uri};

    static LV2_URID map_uri(LV2_URID_Map_Handle handle, const char *uri)
    {
        Urmap *self = static_cast<Urmap *>(handle);
        auto it = self->ids.find(uri);
        if (it != self->ids.end())
            return it->second;
        const LV2_URID id = static_cast<LV2_URID>(self->ids.size() + 1);
        self->ids[uri] = id;
        return id;
    }

    LV2_URID operator[](const char *uri) { return map_uri(this, uri); }
};

double noteFrequency(int midi)
{
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

/* Same synthetic pluck as the pipeline tests. */
void addPluck(std::vector<float> &buffer, int start, int midi, double amplitude)
{
    const double f0 = noteFrequency(midi);
    const int length = std::min<int>(static_cast<int>(kSampleRate * 0.4),
                                     static_cast<int>(buffer.size()) - start);
    for (int i = 0; i < length; ++i)
    {
        const double t = static_cast<double>(i) / kSampleRate;
        const double attack = std::min(1.0, t / 0.002);
        const double envelope = attack * std::exp(-t / 0.18);
        double sample = 0.0;
        for (int h = 1; h <= 10; ++h)
        {
            const double frequency = f0 * h;
            if (frequency > kSampleRate * 0.45)
                break;
            sample += std::sin(2.0 * M_PI * frequency * t) / h;
        }
        buffer[start + i] += static_cast<float>(amplitude * envelope * sample * 0.3);
    }
}

/* Port indices, matching guitarmidi.ttl. */
enum PortIndex
{
    PORT_IN = 0,
    PORT_MIDIOUT = 1,
    PORT_GAIN = 2,
    PORT_EXPRESSIVITY = 3,
    PORT_ONSETSMOOTHING = 4,
    PORT_OFFSETSMOOTHING = 5,
    PORT_ONSETCONFTHR = 6,
    PORT_OFFSETCONFTHR = 7,
    PORT_ONSETENTHR = 8,
    PORT_OFFSETENTHR = 9,
    PORT_DEBUGDATA = 10,
    PORT_LATENCY = 11,
    PORT_FREEWHEEL = 12
};

struct Event
{
    int64_t frame;
    uint8_t note;
    uint8_t velocity;
    bool on;
};

/* One plugin instance plus everything a host connects to it. */
struct Fixture
{
    const LV2_Descriptor *descriptor = nullptr;
    LV2_Handle instance = nullptr;
    Urmap &host;

    std::vector<uint8_t> midi_buffer = std::vector<uint8_t>(kMidiCapacity);
    std::vector<uint8_t> debug_buffer = std::vector<uint8_t>(kDebugCapacity);
    LV2_Options_Option options[3]{};
    LV2_Feature map_feature{};
    LV2_Feature options_feature{LV2_OPTIONS__options, options};
    float gain = 20.0f;
    float expressivity = 7.0f;
    float onsetSmoothing = 0.3f;
    float offsetSmoothing = 0.3f;
    float onsetThreshold = 0.80f;
    float offsetThreshold = 0.10f;
    float onsetEnergy = -20.0f;
    float offsetEnergy = -20.0f;
    float latency = -1.0f;
    float freewheel = 0.0f;

    std::vector<Event> events;
    int64_t blockStart = 0;
    bool events_in_block = true;

    explicit Fixture(Urmap &urids) : host(urids) {}

    ~Fixture()
    {
        if (instance)
            descriptor->cleanup(instance);
    }

    bool instantiate(int max_block_length, int sample_rate, bool announce_options)
    {
        descriptor = lv2_descriptor(0);
        CHECK_MSG(descriptor != nullptr, "the library must export a plugin");
        if (!descriptor)
            return false;

        map_feature = {LV2_URID__map, &host.map};
        std::vector<const LV2_Feature *> features{&map_feature};

        double rate = sample_rate;
        int32_t block = max_block_length;
        if (announce_options)
        {
            options[0] = {LV2_OPTIONS_INSTANCE, 0, host[LV2_CORE__sampleRate],
                          sizeof(double), host[LV2_ATOM__Double], &rate};
            options[1] = {LV2_OPTIONS_INSTANCE, 0, host[LV2_BUF_SIZE__maxBlockLength],
                          sizeof(int32_t), host[LV2_ATOM__Int], &block};
            options[2] = {LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr};
            features.push_back(&options_feature);
        }
        features.push_back(nullptr);

        instance = descriptor->instantiate(descriptor, sample_rate,
                                           GUITARMIDI_TEST_MODEL_DIR,
                                           features.data());
        CHECK_MSG(instance != nullptr, "instantiate must succeed");
        if (!instance)
            return false;

        auto *midi = reinterpret_cast<LV2_Atom_Sequence *>(midi_buffer.data());
        auto *debug = reinterpret_cast<LV2_Atom_Sequence *>(debug_buffer.data());
        midi->atom.size = kMidiCapacity;
        debug->atom.size = kDebugCapacity;

        descriptor->connect_port(instance, PORT_IN, nullptr); // set per block
        descriptor->connect_port(instance, PORT_MIDIOUT, midi);
        descriptor->connect_port(instance, PORT_GAIN, &gain);
        descriptor->connect_port(instance, PORT_EXPRESSIVITY, &expressivity);
        descriptor->connect_port(instance, PORT_ONSETSMOOTHING, &onsetSmoothing);
        descriptor->connect_port(instance, PORT_OFFSETSMOOTHING, &offsetSmoothing);
        descriptor->connect_port(instance, PORT_ONSETCONFTHR, &onsetThreshold);
        descriptor->connect_port(instance, PORT_OFFSETCONFTHR, &offsetThreshold);
        descriptor->connect_port(instance, PORT_ONSETENTHR, &onsetEnergy);
        descriptor->connect_port(instance, PORT_OFFSETENTHR, &offsetEnergy);
        descriptor->connect_port(instance, PORT_DEBUGDATA, debug);
        descriptor->connect_port(instance, PORT_LATENCY, &latency);
        descriptor->connect_port(instance, PORT_FREEWHEEL, &freewheel);

        descriptor->activate(instance);
        return true;
    }

    void prepare_buffers()
    {
        // The plugin forges over the whole buffer, using atom.size as the
        // capacity; hosts reset it before every run.
        auto *midi = reinterpret_cast<LV2_Atom_Sequence *>(midi_buffer.data());
        auto *debug = reinterpret_cast<LV2_Atom_Sequence *>(debug_buffer.data());
        midi->atom.size = kMidiCapacity;
        debug->atom.size = kDebugCapacity;
    }

    void collect_events()
    {
        auto *midi = reinterpret_cast<LV2_Atom_Sequence *>(midi_buffer.data());
        const LV2_URID midi_event = host[LV2_MIDI__MidiEvent];
        LV2_ATOM_SEQUENCE_FOREACH(midi, ev)
        {
            if (ev->body.type != midi_event || ev->body.size < 3)
                continue;
            const uint8_t *msg = reinterpret_cast<const uint8_t *>(LV2_ATOM_BODY(&ev->body));
            if ((msg[0] & 0xF0) != 0x90)
                continue;
            events.push_back({blockStart + static_cast<int64_t>(ev->time.frames),
                              msg[1], msg[2], msg[2] > 0});
        }
    }

    /** Feeds `audio` in blocks of `block_size`; paced to real time if
     * `realtime`. Returns the number of note-ons for `pitches`. */
    int run_phrase(const std::vector<float> &audio, int block_size, bool realtime,
                   const std::vector<int> &pitches, const std::vector<int> &onsets,
                   int upto = -1)
    {
        const int total = upto < 0 ? static_cast<int>(audio.size()) : upto;
        std::vector<float> block(block_size, 0.0f);
        const auto startTime = std::chrono::steady_clock::now();
        int processed = 0;
        int detected = 0;
        std::vector<char> matched(pitches.size(), 0);
        while (processed < total)
        {
            const int n = std::min(block_size, total - processed);
            std::fill(block.begin(), block.end(), 0.0f);
            std::memcpy(block.data(), audio.data() + processed, n * sizeof(float));
            if (realtime)
            {
                const auto due = startTime +
                                 std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                     std::chrono::duration<double>(
                                         static_cast<double>(processed) / kSampleRate));
                std::this_thread::sleep_until(due);
            }
            prepare_buffers();
            blockStart = processed;
            descriptor->connect_port(instance, PORT_IN, block.data());
            const size_t seen = events.size();
            descriptor->run(instance, n);
            collect_events();
            for (size_t e = seen; e < events.size(); ++e)
                if (events[e].frame < blockStart || events[e].frame - blockStart >= n)
                    events_in_block = false;
            processed += n;

            for (size_t i = 0; i < pitches.size(); ++i)
            {
                if (matched[i])
                    continue;
                const int windowEnd = (i + 1 < onsets.size())
                                          ? onsets[i + 1]
                                          : onsets[i] + static_cast<int>(0.45 * kSampleRate);
                for (const Event &event : events)
                {
                    if (event.on && event.note == pitches[i] &&
                        event.frame >= onsets[i] && event.frame < windowEnd)
                    {
                        matched[i] = 1;
                        ++detected;
                        break;
                    }
                }
            }
        }
        return detected;
    }
};

std::vector<int> make_phrase(std::vector<float> &audio, const std::vector<int> &pitches,
                             double lead_in, double spacing)
{
    audio.assign(static_cast<int>((lead_in + pitches.size() * spacing + 0.5) * kSampleRate), 0.0f);
    std::vector<int> onsets;
    for (size_t i = 0; i < pitches.size(); ++i)
    {
        const int start = static_cast<int>((lead_in + i * spacing) * kSampleRate);
        onsets.push_back(start);
        addPluck(audio, start, pitches[i], 0.25);
    }
    return onsets;
}

uint32_t expected_latency(int rate, int max_block_length)
{
    const uint32_t hops = PIPELINE_LATENCY_HOPS - 1 +
                          static_cast<uint32_t>((max_block_length + BUFFER_SIZE - 1) / BUFFER_SIZE);
    return static_cast<uint32_t>(((1 + hops) * BUFFER_SIZE * static_cast<uint64_t>(rate) +
                                  NATIVE_SAMPLERATE / 2) /
                                 NATIVE_SAMPLERATE);
}

} // namespace

int main()
{
    Urmap host;
    const std::vector<int> pitches = {40, 45, 50};
    const double spacing = 0.7;

    // The declared latency must reflect the announced sample rate and block
    // size, and fall back to the base bound without options.
    for (auto [rate, block, announce] : std::vector<std::tuple<int, int, bool>>{
             {48000, 256, true}, {44100, 256, true}, {48000, 1024, true}, {48000, 256, false}})
    {
        Fixture f(host);
        if (!f.instantiate(block, rate, announce))
            return 1;
        std::vector<float> audio(static_cast<int>(0.1 * kSampleRate), 0.0f);
        f.prepare_buffers();
        f.blockStart = 0;
        f.descriptor->connect_port(f.instance, PORT_IN, audio.data());
        f.descriptor->run(f.instance, static_cast<uint32_t>(audio.size()));
        CHECK_MSG(static_cast<uint32_t>(f.latency) ==
                      (announce ? expected_latency(rate, block)
                                : expected_latency(kSampleRate, BUFFER_SIZE)),
                  "latency port must match the announced rate and block size");
        std::printf("latency: rate %d block %d announced %d -> port %u (expected %u)\n",
                    rate, block, announce ? 1 : 0, static_cast<uint32_t>(f.latency),
                    announce ? expected_latency(rate, block)
                             : expected_latency(kSampleRate, BUFFER_SIZE));
        f.descriptor->deactivate(f.instance);
    }

    // Free-wheeling: an unpaced (offline) run must keep every note via the
    // port, at host block sizes from one sample up.
    for (int block : {1, 16, 256, 1024})
    {
        Fixture f(host);
        if (!f.instantiate(block, kSampleRate, true))
            return 1;
        f.freewheel = 1.0f;
        std::vector<float> audio;
        const std::vector<int> onsets = make_phrase(audio, pitches, 0.35, spacing);
        const int detected = f.run_phrase(audio, block, false, pitches, onsets);
        CHECK_MSG(detected == static_cast<int>(pitches.size()),
                  "free-wheeling offline render must keep every note at block size " +
                      std::to_string(block));
        bool ordered = true;
        int64_t previous = -1;
        for (const Event &event : f.events)
        {
            if (event.frame < previous)
                ordered = false;
            previous = event.frame;
        }
        CHECK_MSG(ordered, "events out of order at block size " + std::to_string(block));
        CHECK_MSG(f.events_in_block,
                  "event outside its block at block size " + std::to_string(block));
        f.descriptor->deactivate(f.instance);
        std::printf("freewheel block %4d: detected %d/%zu, events %zu\n",
                    block, detected, pitches.size(), f.events.size());
    }

    // Without free-wheeling, an unpaced run is a real-time render: frames
    // are refused by design and nothing must be emitted.
    {
        Fixture f(host);
        if (!f.instantiate(256, kSampleRate, true))
            return 1;
        std::vector<float> audio;
        const std::vector<int> onsets = make_phrase(audio, pitches, 0.35, spacing);
        const int detected = f.run_phrase(audio, 256, false, pitches, onsets);
        CHECK_MSG(detected == 0 && f.events.empty(),
                  "an unpaced run without free-wheeling must not emit events");
        f.descriptor->deactivate(f.instance);
        std::printf("realtime policy offline: detected %d, events %zu\n",
                    detected, f.events.size());
    }

    // The real-time path through the wrapper at a small block size.
    {
        Fixture f(host);
        if (!f.instantiate(16, kSampleRate, true))
            return 1;
        std::vector<float> audio;
        const std::vector<int> onsets = make_phrase(audio, pitches, 0.35, spacing);
        const int detected = f.run_phrase(audio, 16, true, pitches, onsets);
        CHECK_MSG(detected == static_cast<int>(pitches.size()),
                  "real-time run at block size 16 must detect the phrase");
        f.descriptor->deactivate(f.instance);
        std::printf("realtime block 16: detected %d/%zu, events %zu\n",
                    detected, pitches.size(), f.events.size());
    }

    // A deactivate/activate cycle through the descriptor: the plugin must
    // keep working afterwards (the worker restart).
    {
        Fixture f(host);
        if (!f.instantiate(256, kSampleRate, true))
            return 1;
        f.freewheel = 1.0f;
        std::vector<float> audio;
        const std::vector<int> onsets = make_phrase(audio, pitches, 0.35, spacing);
        const int half = static_cast<int>(audio.size()) / 2 + onsets[0] -
                         static_cast<int>(0.35 * kSampleRate);
        const int before = f.run_phrase(audio, 256, false, pitches, onsets, half);
        f.descriptor->deactivate(f.instance);
        f.descriptor->activate(f.instance);
        const int after = f.run_phrase(audio, 256, false, pitches, onsets);
        CHECK_MSG(before + after >= static_cast<int>(pitches.size()),
                  "a deactivate/activate cycle must leave the plugin working");
        std::printf("lifecycle: detected %d before, %d after the cycle\n", before, after);
    }

    // The free-wheeling port is connectionOptional: leaving it at NULL must
    // be safe.
    {
        Fixture f(host);
        if (!f.instantiate(256, kSampleRate, true))
            return 1;
        f.descriptor->connect_port(f.instance, PORT_FREEWHEEL, nullptr);
        f.freewheel = 1.0f; // must be ignored while the port is NULL
        std::vector<float> audio;
        const std::vector<int> onsets = make_phrase(audio, pitches, 0.35, spacing);
        const int detected = f.run_phrase(audio, 256, false, pitches, onsets);
        CHECK_MSG(detected == 0 && f.events.empty(),
                  "a NULL free-wheeling port must keep the real-time drop policy");
        f.descriptor->deactivate(f.instance);
        std::printf("null freewheel port: detected %d, events %zu\n",
                    detected, f.events.size());
    }

    if (failures == 0)
        std::printf("test_lv2_wrapper: OK\n");
    else
        std::printf("test_lv2_wrapper: %d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
