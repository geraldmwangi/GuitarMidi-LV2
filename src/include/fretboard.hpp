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
#include <filterbank.hpp>
#include <fretboardrepresentation.hpp>
#include <memory>
#include <vector>
#include <map>
#include <noteinferencer.hpp>
#include <config.hpp>
typedef enum
{
    FRETBOARD_INPUT = 0,
    FRETBOARD_MIDIOUTPUT=1,
    FRETBOARD_INPUT_GAIN=2,
    FRETBOARD_SMOOTHING=3,
    FRETBOARD_SMOOTHING_OFFSET=4,
    FRETBOARD_ONSET_THRESHOLD=5,
    FRETBOARD_OFFSET_THRESHOLD=6,
    FRETBOARD_ONSET_ENERGY_THRESHOLD=7,
    FRETBOARD_OFFSET_ENERGY_THRESHOLD=8,
    FRETBOARD_AUDIO_OUTPUT=9
} PortIndex;
using namespace std;
using namespace GuitarMidi;
/**
 * @brief FretBoard holds a filterbank and a noteinferencer. It is responsible for setting up the filterbank based on the fretboard representation and processing the audio input
 * in polyphonic audio. The parameters of the noteinferencer are: smoothing, smoothing offset, onset threshold and offset threshold are set in the fretboard class and can be controlled by the user. The output of the noteinferencer is sent to the midi output buffer.
 * 
 */
class FretBoard
{
private:
   

    GuitarMidi::FretBoardRepresentation m_fretboard_rep;



    FilterBank m_filterbank;
    NoteInferencer m_noteinferencer;

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





    /**
     * @brief Set the Midi Output buffer
     * 
     * @param output 
     */
    void setMidiOutput(LV2_Atom_Sequence *output);



    void setSmoothing(float* smoothing){
        m_noteinferencer.setSmoothing(smoothing);
    }

    void setOnsetThreshold(float* threshold){
        m_noteinferencer.setOnsetThreshold(threshold);
    }

    void setOffsetThreshold(float* threshold){
        m_noteinferencer.setOffsetThreshold(threshold);
    }

    void setSmoothingOffset(float* smoothing_offset){
        m_noteinferencer.setSmoothingOffset(smoothing_offset);
    } 

    void setOnsetEnergyThreshold(float* threshold){
        m_noteinferencer.setOnsetEnergyThreshold(threshold);
    }
    void setOffsetEnergyThreshold(float* threshold){
        m_noteinferencer.setOffsetEnergyThreshold(threshold);
    }  
    void setGain(float* gain_db){
        m_filterbank.setGain(gain_db);
    }
#ifdef WITH_AUDIO_OUTPUT
    void setAudioOutputBuffer(float *output)
    {
        m_noteinferencer.setAudioOutputBuffer(output);
    }
#endif

    /**
     * @brief initialize the filterbank
     * 
     */
    bool initialize(const std::string& bundle_path,float samplerate,int buffer_size);

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
