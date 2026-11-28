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
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <guitarmidi/fretboard_api.hpp>
#include <guitarmidi/common.hpp>
#include <guitarmidi/config.hpp>
#include <guitarmidi/modelinferencer.hpp>

namespace GuitarMidi
{
    /**
     * Converts model note predictions into MIDI events.
     *
     * Runs on the audio thread. Once per analysis hop the caller hands it a
     * contiguous filter-bank frame; the frame is queued for the inference
     * thread and results are emitted under a bounded schedule:
     *
     *  - every result carries the hop index it was computed from,
     *  - a result is emitted at the first completed hop boundary after it
     *    arrives, so a worker that keeps up keeps the fast path instead of
     *    paying a fixed hold,
     *  - a result for hop N that is not in hand by its scheduled hop has
     *    missed the schedule and is counted as late. It still updates the
     *    smoothing state and may release notes, but it cannot start one: a
     *    note-on is never emitted after the bound reported to the host as
     *    lv2:latency. The bound is PIPELINE_LATENCY_HOPS hops plus the host
     *    block size in hops, because a result can only be emitted at a
     *    process() boundary however fast the worker is. A single worker
     *    stall therefore delays the state by a few hops instead of cutting
     *    and re-triggering every held note,
     *  - a short hole in the result stamps (a refused frame on either queue)
     *    is bridged: the one-pole smoothers simply skip the missing hops. A
     *    hole longer than PIPELINE_MAX_BRIDGED_HOPS means the held notes no
     *    longer describe the audio, so they are released and the smoothers
     *    reset.
     */
    class NoteInferencer
    {
        /* The pending ring holds the results drained in one process() call;
         * setHostBufferSize() sizes it for the host block. */
        int m_maxPending = PIPELINE_LATENCY_HOPS + 4;

        struct PendingResult
        {
            uint64_t stamp = 0;
            float probabilities[NUM_NOTES] = {};
            float energies[NUM_NOTES] = {};
        };

        ModelInferencer m_model;

        GuitarMidiOutput *m_midioutput = nullptr;
        float *m_gain_db = nullptr;
        float *m_expressivity_db = nullptr;
        float *m_onset_threshold = nullptr;
        float *m_offset_threshold = nullptr;
        float *m_onset_energy_threshold = nullptr;
        float *m_offset_energy_threshold = nullptr;
        float *m_smoothing = nullptr;
        float *m_smoothing_offset = nullptr;
        float smoothed_onsetoutput[NUM_NOTES] = {0};
        float smoothed_offsetoutput[NUM_NOTES] = {0};
        float smoothed_noteenergies[NUM_NOTES] = {0};
        float smoothed_offsetnoteenergies[NUM_NOTES] = {0};
        float m_momentary_confidences[NUM_NOTES] = {0};
        float m_momentary_noteenergies[NUM_NOTES] = {0};

        int m_backpressureMs = 0;
        int m_maxAgeHops = PIPELINE_LATENCY_HOPS;
        uint64_t m_hopsProcessed = 0;
        uint64_t m_lastResultStamp = 0;
        bool m_haveLastResultStamp = false;
        bool m_finalized = true;
        std::string m_bundle_path;
        const char *m_model_data = nullptr;
        std::size_t m_model_size = 0;
        uint64_t m_staleDropped = 0;
        uint64_t m_late = 0;
        uint64_t m_streamGaps = 0;
        uint64_t m_flushes = 0;
        uint64_t m_noteOns = 0;

        std::vector<PendingResult> m_pending = std::vector<PendingResult>(PIPELINE_LATENCY_HOPS + 4);
        int m_pendingHead = 0;
        int m_pendingCount = 0;

        std::atomic<bool> m_note_on[NUM_NOTES];
        /* Notes still sounding downstream when the plugin was deactivated.
         * deactivate() cannot write MIDI, so their note-offs go out at the
         * start of the next process() call. */
        bool m_releaseOnResume[NUM_NOTES] = {};
        bool m_haveReleases = false;

        /** Moves finished results into the pending queue and notices gaps. */
        void drainResults(int64_t pos);
        /**
         * Applies the thresholds for one result and emits MIDI at `pos`.
         * With `allowOnsets` false (a late result) the state is updated and
         * notes may be released, but no note is started.
         */
        void applyResult(const float *probabilities, const float *energies, int64_t pos,
                         bool allowOnsets = true);
        /** Emits the results that are in hand and whose hop has completed. */
        void emitDueResults(int64_t pos);
        /** Releases every held note and clears the smoothing state. */
        void flushHeldNotes(int64_t pos);
        /** Clears the smoothing and note state without emitting MIDI. */
        void resetDetectionState();

    public:
        /** Creates a note inferencer connected to the supplied MIDI output. */
        NoteInferencer(GuitarMidiOutput *midioutput);
        /** Releases the MIDI output owned by the inferencer. */
        ~NoteInferencer()
        {
            if (m_midioutput != nullptr)
                delete m_midioutput;
        }

        /** Loads and initializes the neural-network model. */
        bool initialize(const std::string &bundle_path);
        /** Loads the model from an embedded in-memory buffer. */
        bool initializeFromBuffer(const char *model_data, std::size_t model_size);
        /**
         * Restarts the inference worker after finalize() (an LV2
         * deactivate/activate cycle). Returns false if the model could not
         * be reloaded; a no-op if the worker is still running.
         */
        bool reactivate();
        /**
         * Stops the inference worker (LV2 deactivate). Held notes are
         * remembered and released at the start of the next process() call.
         */
        void finalize();
        /** Returns the MIDI output used for inferred notes. */
        GuitarMidiOutput *getMidiOutput() { return m_midioutput; }

        /** Sets the gain control. */
        void setGain(float *gain) { m_gain_db = gain; }
        /** Sets the MIDI expressivity control. */
        void setExpressivity(float *expressivity) { m_expressivity_db = expressivity; }
        /** Sets the note-on threshold control. */
        void setOnsetThreshold(float *threshold) { m_onset_threshold = threshold; }
        /** Sets the note-off threshold control. */
        void setOffsetThreshold(float *threshold) { m_offset_threshold = threshold; }
        /** Sets the onset smoothing control. */
        void setSmoothing(float *smoothing) { m_smoothing = smoothing; }
        /** Sets the offset smoothing control. */
        void setSmoothingOffset(float *smoothing_offset) { m_smoothing_offset = smoothing_offset; }
        /** Sets the note-on energy threshold control. */
        void setOnsetEnergyThreshold(float *threshold) { m_onset_energy_threshold = threshold; }
        /** Sets the note-off energy threshold control. */
        void setOffsetEnergyThreshold(float *threshold) { m_offset_energy_threshold = threshold; }

        /**
         * Tells the inferencer the host's maximum block size. Results can
         * only be emitted at process() boundaries: inside one call,
         * consecutive hops are separated by CPU time, not audio time, so a
         * worker that comfortably keeps up with real time can still only be
         * drained at the next call. The scheduling bound therefore grows
         * with the block size (see m_maxAgeHops), and so do the queues: one
         * call queues a whole block of hops back to back.
         *
         * Allocates; call before initialize().
         */
        void setHostBufferSize(int buffer_size);

        /** Testing/diagnostics hook: lower bound for one inference, in ms. */
        void setInferenceDelayMs(int delay_ms) { m_model.setInferenceDelayMs(delay_ms); }
        /** Opt-in producer wait for room (offline rendering); 0 = never wait. */
        void setBackpressureMs(int milliseconds)
        {
            m_backpressureMs = milliseconds;
            m_model.setBackpressureMs(milliseconds);
        }

        /**
         * Starts a MIDI output sequence before processing a block, and
         * releases the notes that were held when the plugin was deactivated.
         */
        void preprocess();

        /** Completes a MIDI output sequence after processing a block. */
        void postprocess()
        {
            if (m_midioutput)
                m_midioutput->finalizeSequence();
        }

        /**
         * Called once per completed analysis hop with the contiguous,
         * oldest-first filter-bank frame of that hop (NUM_FILTERS x
         * BUFFER_SIZE). `pos` is the frame offset within the host block at
         * which the hop ended.
         */
        void processHop(const float *frame, int64_t pos);

        /** Worst-case pipeline delay in frames, including the analysis
         * window: the scheduling bound for the configured host block size
         * (768 frames at blocks up to 256 samples, 1536 at 1024). */
        constexpr uint32_t latencyFrames() const
        {
            return (1 + m_maxAgeHops) * BUFFER_SIZE;
        }

        /** Copies the latest smoothed and momentary confidence and energy values. */
        void getDebugData(float *smoothed_confidences, float *smoothed_energies,
                          float *momentary_confidences, float *momentary_energies,
                          bool *note_on = nullptr,
                          float *smoothed_offset_confidences = nullptr,
                          float *smoothed_offset_energies = nullptr) const;

        /** Pipeline counters and inference timings. */
        NoteInferenceStats getStats() const;

#ifdef WITH_AUDIO_OUTPUT
        float *audio_output;
        /** Sets the optional audio output buffer. */
        void setAudioOutputBuffer(float *output) { audio_output = output; }
#endif
    };
} // namespace GuitarMidi
