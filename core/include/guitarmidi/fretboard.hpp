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
#include <guitarmidi/filterbank.hpp>
#include <guitarmidi/fretboardrepresentation.hpp>
#include <memory>
#include <vector>
#include <map>
#include <guitarmidi/noteinferencer.hpp>
#include <guitarmidi/config.hpp>
#include <zita-resampler/resampler.h>
#include <guitarmidi/fretboard_api.hpp>

using namespace std;
using namespace GuitarMidi;
/**
 * @brief FretBoard holds a filterbank and a noteinferencer. It is responsible for setting up the filterbank based on the fretboard representation and processing the audio input
 * in polyphonic audio. The parameters of the noteinferencer are: smoothing, smoothing offset, onset threshold, offset threshold, onset energy threshold and offset energy threshold. The gain of the filters in the filterbank can also be controlled from the host.
 *  The audio output of the filters in the filterbank can be sent to the host for visualization or debugging purposes. 
 * The note select and harmonic select controls can be used to select which notes and harmonics are active in the filterbank output for visualization or debugging purposes.
 * When the samplerate of the host is different from the native samplerate of the plugin, the audio input is resampled to the native samplerate before being processed by the filterbank and noteinferencer. 
 * The resampled audio is stored in a separate buffer and processed by the plugin. 
 * 
 */
class FretBoard:public FretBoardAPI
{
private:
   

    GuitarMidi::FretBoardRepresentation m_fretboard_rep;



    FilterBank m_filterbank;
    NoteInferencer m_noteinferencer;
    int m_samplerate=48000;
    Resampler m_resampler;
    float* m_input_buffer=nullptr;
    int m_resample_buffer_size=0;
    float* m_resampled_buffer=nullptr;

    void process_resampled(int nsamples);
    void process_direct(int nsamples);

public:
    /**
     * @brief Construct a new Fret Board object. Setup the bank of NoteClassifiers at the standard E A D g b e tuning of the guitar up to the 5th fret

     */
    FretBoard();
    virtual ~FretBoard(){

        if(m_resampled_buffer){
            delete[] m_resampled_buffer;
        }
    };


    /**
     * @brief Set the Audio Input buffer
     * 
     * @param input 
     */
    virtual void setAudioInput(const float *input);





    /**
     * @brief Set the Midi Output buffer
     * 
     * @param output 
     */
    virtual void setMidiOutput(LV2_Atom_Sequence *output);



    virtual void setSmoothing(float* smoothing){
        m_noteinferencer.setSmoothing(smoothing);
    }

    virtual void setOnsetThreshold(float* threshold){
        m_noteinferencer.setOnsetThreshold(threshold);
    }

    virtual void setOffsetThreshold(float* threshold){
        m_noteinferencer.setOffsetThreshold(threshold);
    }

    virtual void setSmoothingOffset(float* smoothing_offset){
        m_noteinferencer.setSmoothingOffset(smoothing_offset);
    } 

    virtual void setOnsetEnergyThreshold(float* threshold){
        m_noteinferencer.setOnsetEnergyThreshold(threshold);
    }
    virtual void setOffsetEnergyThreshold(float* threshold){
        m_noteinferencer.setOffsetEnergyThreshold(threshold);
    }  
    virtual void setGain(float* gain_db){
        m_filterbank.setGain(gain_db);
        m_noteinferencer.setGain(gain_db);
    }
    virtual void setExpressivity(float* expressivity_db){
        m_noteinferencer.setExpressivity(expressivity_db);
    }
#ifdef WITH_AUDIO_OUTPUT
    virtual void setAudioOutputBuffer(float *output)
    {
        m_noteinferencer.setAudioOutputBuffer(output);
    }

    virtual void setFilterOutputBuffer(float *output)
    {
        m_filterbank.setAudioOutputBuffer(output);
    }
    virtual void setNoteSelectControl(float* note_select_buffer){
        m_filterbank.setNoteSelectControl(note_select_buffer);
    }
    virtual void setHarmonicSelectControl(float* harmonic_select_buffer){
        m_filterbank.setHarmonicSelectControl(harmonic_select_buffer);
    }
#endif

    /**
     * @brief initialize the filterbank
     * 
     */
    virtual bool initialize(const std::string& bundle_path,int samplerate,int buffer_size);

    /**
     * @brief finalize all filters and release allocated resources
     * 
     */
    virtual void finalize();

    /**
     * @brief process audio with the filterbank and noteinferencer. The audio is processed in blocks of the size of the host buffer size. 
     * If the samplerate of the host is different from the native samplerate of the plugin, the audio is resampled before being processed by the filterbank and noteinferencer. 
     * The output of the noteinferencer is sent to the midi output buffer as MIDI messages. 
     * 
     * @param nsamples 
     */
    virtual void process(int nsamples);
};


