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
#include <time.h>
#include <noteclassifier.hpp>
#include <harmonicgroup.hpp>
#include <memory>
#include <vector>
#include <map>

typedef enum
{
    FRETBOARD_INPUT = 0,
    FRETBOARD_OUTPUT = 1,
    FRETBOARD_MIDIOUTPUT=2,
    FRETBOARD_POLYPHONIC_TOGGLE=3
} PortIndex;
using namespace std;

/**
 * @brief FretBoard holds a bank of NoteClassifiers, which independently trigger a midi note when finding a fundamental (or a partial) frequency
 * in polyphonic audio
 * 
 */
class FretBoard
{
private:
     float*  m_polyphonic_detection;

    /**
     * @brief m_noteClassifiers: The filterbank
     * 
     */
   

    vector<shared_ptr<NoteClassifier>> m_noteClassifiers;

    map<float,shared_ptr<HarmonicGroup> > m_harmonicGroups;

    shared_ptr<GuitarMidi::MidiOutput> m_midioutput;

    void addNoteClassifier(float freq,float mult,LV2_URID_Map *map, float samplerate);

public:
    /**
     * @brief Construct a new Fret Board object. Setup the bank of NoteClassifiers at the standard E A D g b e tuning of the guitar up to the 5th fret
     * 
     * @param map 
     * @param samplerate 
     */
    FretBoard(LV2_URID_Map *map, float samplerate);


    /**
     * @brief Set the Audio Input buffer
     * 
     * @param input 
     */
    void setAudioInput(const float *input);

    vector<shared_ptr<NoteClassifier>>& getNoteClassifiers()
    {
        return m_noteClassifiers;
    }

    /**
     * @brief Set the Audio Output buffer. This buffer is only used as an internal buffer until I know how to query lv2 for the framebuffersize
     * 
     * @param output 
     */
    void setAudioOutput(float *output);

    /**
     * @brief Set the Midi Output buffer
     * 
     * @param output 
     */
    void setMidiOutput(LV2_Atom_Sequence *output);


    void setPolyPhonicSwitch(float* polyphonic){
        m_polyphonic_detection=polyphonic;

    }

    /**
     * @brief initialize the filterbank
     * 
     */
    void initialize();

    /**
     * @brief finalize all filters and release allocated resources
     * 
     */
    void finalize();

    /**
     * @brief process audio with all NoteClassifiers
     * 
     * @param nsamples 
     */
    void process(int nsamples);
};