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
#pragma once
#include <cstddef>


#include <string>
#include <cstdint>
#include <guitarmidi/common.hpp>
/**
 * Interface for writing MIDI events produced by the fretboard processor.
 */
class GuitarMidiOutput
{
private:
public:
    /** Creates a MIDI output adapter. */
    GuitarMidiOutput() {};
    /** Releases the MIDI output adapter. */
    virtual ~GuitarMidiOutput() {};
    /** Starts a MIDI event sequence. */
    virtual void initializeSequence()=0;
    /** Completes a MIDI event sequence. */
    virtual void finalizeSequence()=0;
    /** Sends a three-byte MIDI message at the specified audio frame. */
    virtual void sendMidiMessage(uint8_t midinote[3], int64_t frames)=0;
};


/**
 * Interface for configuring and processing the guitar fretboard audio pipeline.
 */
class FretBoardAPI
{

  

public:
    /**
     * @brief Construct a new Fret Board object. Setup the bank of NoteClassifiers at the standard E A D g b e tuning of the guitar up to the 5th fret
     *
     * @param map
     * @param samplerate
     */
    /** Creates the fretboard processing interface. */
    FretBoardAPI() { };
    /** Releases the fretboard processing interface. */
    virtual ~FretBoardAPI() {};

    /**
     * @brief Set the Audio Input buffer
     *
     * @param input
     */
    virtual void setAudioInput(const float *input) = 0;

    /**
     * @brief Set the Midi Output buffer
     *
     * @param output
     */
    /** Returns the MIDI output used by the processor. */
    virtual GuitarMidiOutput *getMidiOutput() =0;

    /** Sets the smoothing control. */
    virtual void setSmoothing(float *smoothing) = 0;

    /** Sets the note-on threshold control. */
    virtual void setOnsetThreshold(float *threshold) = 0;

    /** Sets the note-off threshold control. */
    virtual void setOffsetThreshold(float *threshold) = 0;

    /** Sets the smoothing time used for note-off detection. */
    virtual void setSmoothingOffset(float *smoothing_offset) = 0;

    /** Sets the note-on energy threshold control. */
    virtual void setOnsetEnergyThreshold(float *threshold) = 0;
    /** Sets the note-off energy threshold control. */
    virtual void setOffsetEnergyThreshold(float *threshold) = 0;
    /** Sets the filter and inference gain control. */
    virtual void setGain(float *gain_db) = 0;
    /** Sets the MIDI expressivity control. */
    virtual void setExpressivity(float *expressivity_db) = 0;
#ifdef WITH_AUDIO_OUTPUT
    /** Sets the optional audio output buffer. */
    virtual void setAudioOutputBuffer(float *output) = 0;

    /** Sets the optional filter output buffer. */
    virtual void setFilterOutputBuffer(float *output) = 0;
    /** Sets the optional note-selection control buffer. */
    virtual void setNoteSelectControl(float *note_select_buffer) = 0;
    /** Sets the optional harmonic-selection control buffer. */
    virtual void setHarmonicSelectControl(float *harmonic_select_buffer) = 0;
#endif

    /**
     * @brief initialize the filterbank
     *
     */
    virtual bool initialize(const std::string &bundle_path, int samplerate, int buffer_size) = 0;
    virtual bool initializeFromBuffer(const char*, std::size_t, int, int) { return false; }

    /**
     * @brief finalize all filters and release allocated resources
     *
     */
    virtual void finalize() = 0;

    /**
     * @brief restart the inference worker after finalize()
     *
     * LV2 hosts deactivate and re-activate instances around buffer-size
     * changes and graph edits; finalize() stops the worker, so activate
     * must bring it back. Returns false if the model could not be reloaded.
     */
    virtual bool reactivate() = 0;

    /**
     * @brief process audio with the filterbank and noteinferencer. The audio is processed in blocks of the size of the host buffer size.
     * If the samplerate of the host is different from the native samplerate of the plugin, the audio is resampled before being processed by the filterbank and noteinferencer.
     * The output of the noteinferencer is sent to the midi output buffer as MIDI messages.
     *
     * @param nsamples
     */
    virtual void process(int nsamples) = 0;

    /** Constant pipeline delay in frames, for the host's latency compensation. */
    virtual uint32_t getLatencyFrames() const = 0;

    /** Pipeline counters and inference timings. */
    virtual GuitarMidi::NoteInferenceStats getInferenceStats() const = 0;

    /**
     * Opt-in producer wait when the inference queue is full, in milliseconds.
     * 0 is the real-time policy (drop the frame and report the gap); a
     * positive value is for offline rendering, where waiting keeps every note.
     */
    virtual void setBackpressureMs(int milliseconds) = 0;

    /** Copies the latest smoothed and momentary note confidence and energy values. */
    virtual void getNoteDebugData(float *smoothed_confidences, float *smoothed_energies,
                                  float *momentary_confidences, float *momentary_energies,
                                  bool *note_on = nullptr,
                                  float *smoothed_offset_confidences = nullptr,
                                  float *smoothed_offset_energies = nullptr) const = 0;

    /** Copies decimated signed waveform samples for every filter. */
    virtual void getFilterWaveforms(float *waveforms, uint32_t samples_per_filter) const = 0;
};

FretBoardAPI *creatFretBoard(GuitarMidiOutput*);
