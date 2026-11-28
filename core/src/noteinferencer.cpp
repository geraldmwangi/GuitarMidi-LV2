#include <guitarmidi/noteinferencer.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>

namespace GuitarMidi
{

    NoteInferencer::NoteInferencer(GuitarMidiOutput *midioutput) : m_midioutput(midioutput)
    {
        for (auto &note_on : m_note_on)
            note_on.store(false, std::memory_order_relaxed);
    }

    bool NoteInferencer::initialize(const std::string &bundle_path)
    {
        if (m_midioutput)
            m_midioutput->initializeSequence();
        m_bundle_path = bundle_path;
        m_model_data = nullptr;
        // A worker that failed to start counts as finalized: reactivate()
        // then retries the load instead of reporting a running pipeline.
        m_finalized = !m_model.initialize(bundle_path);
        return !m_finalized;
    }

    bool NoteInferencer::initializeFromBuffer(const char *model_data, std::size_t model_size)
    {
        if (m_midioutput)
            m_midioutput->initializeSequence();
        m_bundle_path.clear();
        m_model_data = model_data;
        m_model_size = model_size;
        m_finalized = !m_model.initializeFromBuffer(model_data, model_size);
        return !m_finalized;
    }

    bool NoteInferencer::reactivate()
    {
        // finalize() stopped the worker, but LV2 hosts deactivate and
        // activate instances around buffer-size changes, graph edits and
        // latency queries, and expect the plugin to work afterwards. The
        // interpreter is still loaded, so this only restarts the thread; the
        // model is loaded again only if it never loaded in the first place.
        // The stamps continue where the producer is, and
        // m_haveLastResultStamp is cleared so the first result after the
        // restart is not mistaken for a gap.
        if (!m_finalized)
            return true;
        bool ok = m_model.restart();
        if (!ok)
            ok = m_model_data != nullptr
                     ? m_model.initializeFromBuffer(m_model_data, m_model_size)
                     : m_model.initialize(m_bundle_path);
        if (!ok)
            return false;
        m_finalized = false;
        resetDetectionState();
        m_pendingCount = 0;
        m_pendingHead = 0;
        m_haveLastResultStamp = false;
        m_lastResultStamp = 0;
        return true;
    }

    void NoteInferencer::finalize()
    {
        m_model.finalize();
        m_finalized = true;
        // deactivate() cannot write MIDI: remember what is still sounding
        // downstream and release it at the start of the next process().
        for (int i = 0; i < NUM_NOTES; ++i)
        {
            if (m_note_on[i].load(std::memory_order_relaxed))
            {
                m_releaseOnResume[i] = true;
                m_haveReleases = true;
                m_note_on[i].store(false, std::memory_order_relaxed);
            }
        }
        m_pendingCount = 0;
        m_pendingHead = 0;
        if (m_midioutput)
            m_midioutput->finalizeSequence();
    }

    void NoteInferencer::setHostBufferSize(int buffer_size)
    {
        if (buffer_size < 1)
            buffer_size = BUFFER_SIZE;
        const int block_hops = (buffer_size + BUFFER_SIZE - 1) / BUFFER_SIZE;
        m_maxAgeHops = PIPELINE_LATENCY_HOPS - 1 + block_hops;
        // One process() call can drain a whole block of results before any
        // of them is emitted.
        m_maxPending = block_hops + RING_BUFFER_SIZE + PIPELINE_LATENCY_HOPS + 4;
        m_pending.assign(m_maxPending, PendingResult{});
        m_pendingHead = 0;
        m_pendingCount = 0;
        // One call queues block_hops frames back to back; RING_BUFFER_SIZE
        // is the slack for a worker that is momentarily behind.
        m_model.setQueueCapacity(static_cast<std::size_t>(block_hops + RING_BUFFER_SIZE));
    }

    void NoteInferencer::preprocess()
    {
        if (m_midioutput)
            m_midioutput->initializeSequence();
        if (!m_haveReleases)
            return;
        for (int i = 0; i < NUM_NOTES; ++i)
        {
            if (m_releaseOnResume[i])
            {
                uint8_t midinote[3] = {0x90, static_cast<uint8_t>(i + NOTE_OFFSET), 0x00};
                if (m_midioutput)
                    m_midioutput->sendMidiMessage(midinote, 0);
                m_releaseOnResume[i] = false;
            }
        }
        m_haveReleases = false;
    }

    void NoteInferencer::resetDetectionState()
    {
        for (int i = 0; i < NUM_NOTES; ++i)
        {
            m_note_on[i].store(false, std::memory_order_relaxed);
            smoothed_onsetoutput[i] = 0.0f;
            smoothed_offsetoutput[i] = 0.0f;
            smoothed_noteenergies[i] = 0.0f;
            smoothed_offsetnoteenergies[i] = 0.0f;
            m_momentary_confidences[i] = 0.0f;
            m_momentary_noteenergies[i] = 0.0f;
        }
    }

    void NoteInferencer::flushHeldNotes(int64_t pos)
    {
        for (int i = 0; i < NUM_NOTES; ++i)
        {
            if (m_note_on[i].load(std::memory_order_relaxed))
            {
                uint8_t midinote[3] = {0x90, static_cast<uint8_t>(i + NOTE_OFFSET), 0x00};
                if (m_midioutput)
                    m_midioutput->sendMidiMessage(midinote, pos);
                m_note_on[i].store(false, std::memory_order_relaxed);
            }
        }
        resetDetectionState();
    }

    void NoteInferencer::drainResults(int64_t pos)
    {
        float result[NUM_NOTES * 2];
        uint64_t stamp = 0;
        const int capacity = static_cast<int>(m_pending.size());
        while (m_model.get_model_output(result, stamp))
        {
            if (m_haveLastResultStamp && stamp != m_lastResultStamp + 1)
            {
                // The inference stream skipped hops: a frame was refused on
                // the way in or a result on the way out. A short hole is
                // bridged (the smoothers just skip it); a long one means the
                // held notes no longer describe the audio, so release them —
                // after emitting the results from before the hole.
                ++m_streamGaps;
                const bool ordered = stamp > m_lastResultStamp;
                const uint64_t hole = ordered ? stamp - m_lastResultStamp - 1 : 0;
                if (!ordered || hole > PIPELINE_MAX_BRIDGED_HOPS)
                {
                    emitDueResults(pos);
                    flushHeldNotes(pos);
                    m_pendingCount = 0;
                    m_pendingHead = 0;
                    ++m_flushes;
                }
            }
            m_lastResultStamp = stamp;
            m_haveLastResultStamp = true;

            // Results older than the current hop are due anyway: emit them to
            // make room before giving up on one.
            if (m_pendingCount == capacity)
                emitDueResults(pos);
            if (m_pendingCount == capacity)
            {
                // Still full: drop the oldest. A one-hop hole in the emitted
                // stream is bridged like any short hole.
                m_pendingHead = (m_pendingHead + 1) % capacity;
                --m_pendingCount;
                ++m_staleDropped;
                ++m_streamGaps;
            }
            const int slot = (m_pendingHead + m_pendingCount) % capacity;
            m_pending[slot].stamp = stamp;
            std::memcpy(m_pending[slot].probabilities, result,
                        sizeof(float) * NUM_NOTES);
            std::memcpy(m_pending[slot].energies, result + NUM_NOTES,
                        sizeof(float) * NUM_NOTES);
            ++m_pendingCount;
        }
    }

    void NoteInferencer::emitDueResults(int64_t pos)
    {
        while (m_pendingCount > 0)
        {
            PendingResult &pending = m_pending[m_pendingHead];
            const uint64_t age = m_hopsProcessed - pending.stamp;
            // Only the hop that just closed can be age 0 (and only if the
            // worker finished within the same processHop call); a result is
            // never emitted before the window it describes has completed.
            if (age < 1)
                break;

            // A result that missed its scheduled hop still feeds the
            // smoothers and may release notes, but a note-on emitted now
            // would land after the delay declared to the host. Keeping the
            // state is what lets a held note survive a single worker stall
            // instead of being cut and re-triggered.
            const bool late = age > m_maxAgeHops;
            if (late)
                ++m_late;
            applyResult(pending.probabilities, pending.energies, pos, !late);

            m_pendingHead = (m_pendingHead + 1) % static_cast<int>(m_pending.size());
            --m_pendingCount;
        }
    }

    void NoteInferencer::applyResult(const float *output_data, const float *energies, int64_t pos,
                                     bool allowOnsets)
    {
        if (!m_gain_db || !m_expressivity_db || !m_smoothing || !m_smoothing_offset ||
            !m_onset_threshold || !m_offset_threshold ||
            !m_onset_energy_threshold || !m_offset_energy_threshold)
            return;

        int num_active_notes = 0;
        for (int h = 0; h < NUM_NOTES; h++)
            num_active_notes += m_note_on[h].load(std::memory_order_relaxed);

        const float gain = powf(10.0f, *m_gain_db * 0.05f);
        const float expressivity = powf(10.0f, *m_expressivity_db * 0.05f);
        const float en_threshold = pow(10, *m_onset_energy_threshold / 20) * gain;
        const float en_threshold_offset = pow(10, *m_offset_energy_threshold / 20) * gain;

        bool triggeron[NUM_NOTES] = {false};
        bool triggeroff[NUM_NOTES] = {false};
        for (int i = 0; i < NUM_NOTES; i++)
        {
            // The energies travel with the result (computed by the worker
            // from the same analysis window), so the energy gate always
            // matches the probabilities it is applied to.
            const float note_energy = energies[i];

            m_momentary_confidences[i] = output_data[i];
            m_momentary_noteenergies[i] = note_energy;

            smoothed_onsetoutput[i] = *m_smoothing * smoothed_onsetoutput[i] + (1 - *m_smoothing) * output_data[i]; // simple low pass filter to smooth the output and reduce jitter
            smoothed_offsetoutput[i] = *m_smoothing_offset * smoothed_offsetoutput[i] + (1 - *m_smoothing_offset) * output_data[i];
            if (num_active_notes == 1)
            {
                smoothed_noteenergies[i] = *m_smoothing * smoothed_noteenergies[i] + (1 - *m_smoothing) * note_energy * output_data[i];
                smoothed_offsetnoteenergies[i] = *m_smoothing_offset * smoothed_offsetnoteenergies[i] + (1 - *m_smoothing_offset) * note_energy * output_data[i];
            }
            else
            {
                smoothed_noteenergies[i] = *m_smoothing * smoothed_noteenergies[i] + (1 - *m_smoothing) * note_energy;
                smoothed_offsetnoteenergies[i] = *m_smoothing_offset * smoothed_offsetnoteenergies[i] + (1 - *m_smoothing_offset) * note_energy;
            }

            if (!m_note_on[i].load(std::memory_order_relaxed))
            {
                triggeron[i] = allowOnsets && (smoothed_onsetoutput[i] > *m_onset_threshold && smoothed_noteenergies[i] > en_threshold);
            }
            else
            {
                triggeroff[i] = (smoothed_offsetoutput[i] < *m_offset_threshold && smoothed_offsetnoteenergies[i] < en_threshold_offset);
            }
        }
        if (num_active_notes == 1) // fix for bendings in solos
            for (int i = 0; i < (NUM_NOTES - 1); i++)
            {
                if (smoothed_onsetoutput[i] > smoothed_onsetoutput[i + 1])
                {
                    triggeroff[i + 1] = m_note_on[i + 1].load(std::memory_order_relaxed);
                }
                else
                {
                    triggeroff[i] = m_note_on[i].load(std::memory_order_relaxed);
                }
            }
        for (int i = 0; i < NUM_NOTES; i++)
        {
            if (triggeron[i])
            {
                int velocity = (int)(smoothed_noteenergies[i] / (gain * expressivity) * 127);
                // A note-on with velocity 0 is a note-off: keep it audible.
                velocity = std::clamp(velocity, 1, 127);
                uint8_t midinote[3] = {0x90, static_cast<uint8_t>(i + NOTE_OFFSET), static_cast<uint8_t>(velocity)};
                if (m_midioutput)
                    m_midioutput->sendMidiMessage(midinote, pos);
                m_note_on[i].store(true, std::memory_order_relaxed);
                ++m_noteOns;
            }
            if (triggeroff[i] && m_note_on[i])
            {
                uint8_t midinote[3] = {0x90, static_cast<uint8_t>(i + NOTE_OFFSET), 0x00};
                if (m_midioutput)
                    m_midioutput->sendMidiMessage(midinote, pos);
                m_note_on[i].store(false, std::memory_order_relaxed);
            }
        }
    }

    void NoteInferencer::processHop(const float *frame, int64_t pos)
    {
        const uint64_t stamp = ++m_hopsProcessed;
        // No worker (run() after deactivate, or a failed restart): nothing
        // would drain the queue, and the backpressure waits below would burn
        // their whole budget on every hop of an offline render.
        if (!m_model.running())
            return;
        // A refused frame is a hole in the stream; the resulting stamp gap is
        // handled in drainResults once inference catches up.
        m_model.add_audio_input(frame, stamp);

        drainResults(pos);

        // Render mode (backpressure set): the host is not on a clock, so do
        // not let the audio run further ahead of the results than one hop —
        // exactly the distance a real-time run on a machine that keeps up
        // settles at, so an offline render produces the same timing. Without
        // this the producer would race to fill the queue, the results would
        // all arrive older than the schedule and be discarded, and an offline
        // render would come out silent.
        if (m_backpressureMs > 0 && m_hopsProcessed > 1)
        {
            const uint64_t needed = m_hopsProcessed - 1;
            const auto deadline = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(m_backpressureMs);
            while ((!m_haveLastResultStamp || m_lastResultStamp < needed) &&
                   std::chrono::steady_clock::now() < deadline)
            {
                std::this_thread::yield();
                drainResults(pos);
            }
        }

        emitDueResults(pos);
    }

    void NoteInferencer::getDebugData(float *smoothed_confidences, float *smoothed_energies,
                                      float *momentary_confidences, float *momentary_energies,
                                      bool *note_on, float *smoothed_offset_confidences,
                                      float *smoothed_offset_energies) const
    {
        for (int i = 0; i < NUM_NOTES; ++i)
        {
            smoothed_confidences[i] = smoothed_onsetoutput[i];
            smoothed_energies[i] = smoothed_noteenergies[i];
            if (smoothed_offset_confidences != nullptr)
                smoothed_offset_confidences[i] = smoothed_offsetoutput[i];
            if (smoothed_offset_energies != nullptr)
                smoothed_offset_energies[i] = smoothed_offsetnoteenergies[i];
            momentary_confidences[i] = m_momentary_confidences[i];
            momentary_energies[i] = m_momentary_noteenergies[i];
            if (note_on != nullptr)
                note_on[i] = m_note_on[i].load(std::memory_order_relaxed);
        }
    }

    NoteInferenceStats NoteInferencer::getStats() const
    {
        NoteInferenceStats stats;
        stats.hopsProcessed = m_hopsProcessed;
        stats.inputDropped = m_model.inputDropped();
        stats.outputDropped = m_model.outputDropped();
        stats.streamGaps = m_streamGaps;
        stats.staleDropped = m_staleDropped;
        stats.late = m_late;
        stats.flushes = m_flushes;
        stats.invokeFailures = m_model.invokeFailures();
        stats.noteOns = m_noteOns;
        stats.meanInferenceMs = m_model.meanInferenceMs();
        stats.maxInferenceMs = m_model.maxInferenceMs();
        return stats;
    }

} // namespace GuitarMidi
