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
#include <DspFilters/Dsp.h>
#include <aubio/aubio.h>
#include <aubio/pitch/pitch.h>
#include <midioutput.hpp>
#include <memory>

#define FILTERORDER 2 // the real order is 2*MAXORDER
#define MAXORDER 10 // the real order is 2*MAXORDER
//#define USE_ELLIPTIC 1
using namespace std;
/**
 * @brief NoteClassifier analyses polyphonic audio and triggers a midi note when it finds 
 * a signal of a particular frequency
 * 
 * @details NoteClassifier filters incomming audio with a series of elliptic band pass filters with center frequency m_centerfreq
 * If the response of the filters reach a certain threshold then the audio has a candidate partial with frequency f_cand in the band with bandwidth m_bandwidth
 * centered at m_centerfreq. However f_cand doesn't neccessarily match m_centerfreq, it might even correspond to the musical note of a neighboring band.
 * E. g. the bands of notes E (82.41Hz) and F (87.31Hz) overlap. Thus when the E string of the guitar is struck, this triggers the NoteClassifier centered at the note E
 * but also the NoteClassifier at the note F. To prevent this the response of the elliptic filter is anaylised with a monophonic pitch detection. Only when 
 * the resulting pitch matches m_centerfreq within a narrow bound of 4 Hz, NoteClassifier sends a midi note
 *
 */
class NoteClassifier
{
private:

    /**
     * @brief m_centerfreq: The center frequency of the bandpass filters
     * 
     */
    float m_centerfreq;

    /**
     * @brief m_bandwidth: The bandwidth frequency of the bandpass filters
     * 
     */
    float m_bandwidth;

    int m_order;

    /**
     * @brief m_passbandatten: The attenuation in db of amplitude of the ripple in the pass and stopband of the elliptic filters
     * 
     */
    float m_passbandatten;

    /**
     * @brief m_samplerate: The samplerate of the audio
     * 
     */
    float m_samplerate;

    /**
     * @brief mPitchDetector: The pitch detector used to validate the response of the fillters
     * This validation step is probably not need when we compare the responses of neighboring bands against eachother
     * 
     */
    aubio_pitch_t* mPitchDetector;

    aubio_onset_t* m_onsetDetector;

    /**
     * @brief The number of samples since the last onset was detected
     * It is -1 if no note is playing
     * 
     */
    int m_numSamplesSinceLastOnset;



    float m_meanEnv;
    int m_meanEnvCounter;

    float m_onsetThresh;
    float m_onsetSilence;
    float m_onsetCompression;
    int m_onsetBuffersize;
    string m_onsetMethod;

    /**
     * @brief m_pitchfreq: The result of the pitch estimator
     * >
     */
    fvec_t* m_pitchfreq;

    /**
     * @brief m_pitchbuffer: We need to buffer audio for the pitchdetector
     * 
     */
    float* m_pitchbuffer;

    /**
     * @brief m_pitchBufferCounter: Counter to meassure when m_pitchbuffer is ready to be analysed
     * 
     */
    int m_pitchBufferCounter;

    /**
     * @brief mInBufSize: size of m_pitchbuffer
     * 
     */
    int mInBufSize;

    /**
     * @brief mBufferSize: audio buffersize set by the host
     * 
     */
    int mBufferSize;

    /**
     * @brief m_noteOnOffState: The current state of the NoteClassifier. True if a note is being played, false otherwise
     * 
     */
    bool m_noteOnOffState;

    int m_samplesSinceLastChangeOfState;

    /**
     * @brief m_oldNoteOnOffState: The state of NoteClassifier at the previous call of process()
     * 
     */
    bool m_oldNoteOnOffState;

    /**
     * @brief m_filter: array of elliptic filters
     * 
     */
    #ifndef USE_ELLIPTIC
    Dsp::SimpleFilter <Dsp::Butterworth::BandPass<MAXORDER>, 1> m_filter;
    #else
    Dsp::SimpleFilter <Dsp::Elliptic::BandPass<MAXORDER>, 1> m_filter;
    #endif


    /**
     * @brief m_midiOutput: The midioutput object
     * 
     */
    std::shared_ptr<GuitarMidi::MidiOutput> m_midiOutput;

public:
    typedef shared_ptr<NoteClassifier> Ptr;
    /**
     * @brief Construct a new Note Classifier object
     * 
     * @param map Featuremap provided by the lv2 host
     * @param samplerate Samplerate provided by the lv2 host
     * @param center centerfrequency for which the NoteClassifier trigger the corresponding midi note
     * @param bandwidth bandwidth of the bandpass filters
     * @param passbandatten The attenuation in db of amplitude of the ripple in the pass and stopband of the elliptic filters
     */
    NoteClassifier(LV2_URID_Map *map,float samplerate, float center = 110.0, float bandwidth = 20, float passbandatten = 5);

    /**
     * @brief initialize the filters and the pitchdetector
     * 
     */
    void initialize();

    /**
     * @brief reset the filters, delete the pitchdetector
     * 
     */
    void finalize();

    /**
     * @brief process incomming audio
     * 
     * @param nsamples 
     */
    void process(int nsamples);

    bool isNoteValid(int samples);

    bool getNoteOnOffState()
    {
        return m_noteOnOffState;
    }
    float getMeanEnv()
    {
        if(m_meanEnvCounter)
            return m_meanEnv/m_meanEnvCounter;
        else
            return 0.0;
    }

    void setNoteOnOffState(bool state)
    {
        m_noteOnOffState=state;
    }

    void sendMidiNote(int nsamples);

    bool is_ringing;

    bool block_midinote;

    void setIsRinging(int nsamples);

    int getNumSamplesSinceLastOnset()
    {
        return m_numSamplesSinceLastOnset;
    }

    int getNumSamplesSinceLastChangeOfState()
    {
        return m_samplesSinceLastChangeOfState;
    }

    void sendMidiNote(int nsamples, bool noteon);

    /**
     * @brief Filter incomming audio and compute the average response
     * 
     * @param buffer 
     * @param nsamples 
     * @return float 
     */
    float filterAndComputeMeanEnv(float *buffer,int nsamples,bool* onsetdetected=nullptr);

    Dsp::complex_t filterResponse(float freq);

    /**
     * @brief pointer to input audio buffer
     * 
     */
    const float *input;

    /**
     * @brief pointer to output audio buffer
     * 
     */
    float *output;

    /**
     * @brief Set the Midi Output object
     * 
     * @param output pointer to midi output buffer provided by the host
     */
    void setMidiOutput(shared_ptr<GuitarMidi::MidiOutput> output);

    void setOnsetParameter(string method,float threshold=0.4,float silence=-40,float comp=0.0,int onsetbuffersize=512,bool adap_whitening=false);

    void setFilterParameters(float bandwidth = 20, float passbandatten = 1,int order=FILTERORDER);

    void resetFilterAndOnsetDetector();

    float getCenterFrequency()
    {
        return m_centerfreq;
    }
};
