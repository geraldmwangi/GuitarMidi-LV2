#pragma once
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
#include <cstdint>
#include <guitarmidi/logging.hpp>

#define BUFFER_SIZE 256
#define NATIVE_SAMPLERATE 48000
#define NOTE_OFFSET 40
#define NUM_HARMONICS 4
#define NUM_NOTES 37

#define NUM_STRINGS 6
#define NUM_FRETS 12

#define Q_FACTOR 6

/* Scheduling bound of the inference pipeline, in analysis hops (BUFFER_SIZE
 * frames each). A result for hop N is emitted at the first completed hop
 * boundary after it arrives, so a worker that keeps up keeps the fast path.
 * If it is not in hand by its scheduled hop it has missed the schedule: it is
 * counted as late and may only update state and release notes, never start
 * one, so no note-on is emitted after the bound reported to the host.
 *
 * The bound is PIPELINE_LATENCY_HOPS hops for host blocks up to one hop; for
 * larger blocks it grows with the block size (see
 * NoteInferencer::setHostBufferSize), because results can only be emitted at
 * process() boundaries: inside one call consecutive hops are separated by CPU
 * time, not audio time, so even a worker that comfortably keeps up with real
 * time is drained at the next call.
 *
 * Worst-case latency in frames is (1 + bound) * BUFFER_SIZE: the analysis
 * window itself plus the scheduling bound. At 48 kHz with BUFFER_SIZE = 256
 * and 2 hops that is 768 frames ≈ 16 ms; a worker that keeps up emits one hop
 * earlier (512 frames ≈ 10.7 ms).
 */
#define PIPELINE_LATENCY_HOPS 2

/* Longest hole in the result stream (hops whose frame or result was refused)
 * that is bridged without releasing the held notes. The smoothers are
 * one-pole filters, so skipping a few hops only delays them; 8 hops is about
 * 43 ms at 48 kHz. A longer hole releases every held note and resets the
 * detection state, because it no longer describes the audio. */
#define PIPELINE_MAX_BRIDGED_HOPS 8
namespace GuitarMidi
{

    /*
    * A 2D audio buffer structure.
        * num_filters: The number of filters used in the audio processing.
        * window_size: The size of the window used for processing the audio buffer.
        * audio_buffer_2D: A pointer to a 2D array that holds the audio data. The dimensions of this array are determined by num_filters and window_size.

    */
    struct AudioBuffer2D
    {
        int num_filters;
        int window_size;
        float *audio_buffer_2D;
    };

    /* 
    * A structure to represent the filter characteristics for a specific note in the guitar fretboard.
    * filter_id: An integer that uniquely identifies the filter associated with a particular note or its overtones.
    * center_freq: A float that represents the center frequency of the filter
    */
    struct FilterRepresentation
    {
        int filter_id;
        float center_freq;
    };

    /*
    * Observable state of the detection pipeline. Everything here is a counter
    * except the inference timings, so a host or a test can tell whether the
    * pipeline kept up with real time instead of guessing from the audio.
    * inputDropped/outputDropped mean the bounded queues refused a frame;
    * streamGaps counts holes the consumer noticed in the frame stamps;
    * staleDropped counts results lost to a pending-queue overflow;
    * late counts results that missed their scheduled hop: they updated the
    * detection state but could not start a note, so no note-on is ever
    * emitted after the declared latency;
    * flushes counts holes too long to bridge, which released every held
    * note; invokeFailures counts interpreter invocations that failed.
    */
    struct NoteInferenceStats
    {
        uint64_t hopsProcessed = 0;
        uint64_t inputDropped = 0;
        uint64_t outputDropped = 0;
        uint64_t streamGaps = 0;
        uint64_t staleDropped = 0;
        uint64_t late = 0;
        uint64_t flushes = 0;
        uint64_t invokeFailures = 0;
        uint64_t noteOns = 0;
        double meanInferenceMs = 0.0;
        double maxInferenceMs = 0.0;
    };
}