/* GuitarMidi-LV2 Library
 * Copyright (C) 2022 Gerald Mwangi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */
#pragma once


#include <string>

// template <typename T>
// concept FretBoardAPI = requires(
// 	T& fretboard,
// 	const float* audio_input,
// 	LV2_Atom_Sequence* midi_output,
// 	const std::string& bundle_path,
// 	float* control,
// 	int samplerate,
// 	int buffer_size,
// 	int nsamples)
// {
// 	{ fretboard.setAudioInput(audio_input) } -> std::same_as<void>;
// 	{ fretboard.setMidiOutput(midi_output) } -> std::same_as<void>;

// 	{ fretboard.setSmoothing(control) } -> std::same_as<void>;
// 	{ fretboard.setOnsetThreshold(control) } -> std::same_as<void>;
// 	{ fretboard.setOffsetThreshold(control) } -> std::same_as<void>;
// 	{ fretboard.setSmoothingOffset(control) } -> std::same_as<void>;
// 	{ fretboard.setOnsetEnergyThreshold(control) } -> std::same_as<void>;
// 	{ fretboard.setOffsetEnergyThreshold(control) } -> std::same_as<void>;
// 	{ fretboard.setGain(control) } -> std::same_as<void>;
// 	{ fretboard.setExpressivity(control) } -> std::same_as<void>;

// 	{ fretboard.initialize(bundle_path, samplerate, buffer_size) } -> std::same_as<bool>;
// 	{ fretboard.finalize() } -> std::same_as<void>;
// 	{ fretboard.process(nsamples) } -> std::same_as<void>;

// };
/**
 * Interface for writing MIDI events produced by the fretboard processor.
 */
class MidiOutput
{
private:
public:
    /** Creates a MIDI output adapter. */
    MidiOutput() {};
    /** Releases the MIDI output adapter. */
    virtual ~MidiOutput() {};
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
    virtual MidiOutput *getMidiOutput() =0;

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

    /**
     * @brief finalize all filters and release allocated resources
     *
     */
    virtual void finalize() = 0;

    /**
     * @brief process audio with the filterbank and noteinferencer. The audio is processed in blocks of the size of the host buffer size.
     * If the samplerate of the host is different from the native samplerate of the plugin, the audio is resampled before being processed by the filterbank and noteinferencer.
     * The output of the noteinferencer is sent to the midi output buffer as MIDI messages.
     *
     * @param nsamples
     */
    virtual void process(int nsamples) = 0;
};

FretBoardAPI *creatFretBoard(MidiOutput*);
