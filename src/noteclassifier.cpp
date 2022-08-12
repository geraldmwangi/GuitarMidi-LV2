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
#include <noteclassifier.hpp>
#include <cmath>

NoteClassifier::NoteClassifier(LV2_URID_Map *map, float samplerate, float center, float bandwidth, float passbandatten) 
{
    m_centerfreq = center;
    m_passbandatten = passbandatten;
    m_bandwidth = bandwidth;
    m_samplerate = samplerate;
    mPitchDetector = 0;
    m_pitchbuffer = 0;
    m_pitchBufferCounter = 0;
    m_pitchfreq = 0;

    //TODO query host for buffersize. This is only used for the pitch detector
    mPitchBufferSize = 256;

    m_bufferSize=256;
    m_buffer=nullptr;
    m_noteOnOffState = false;
    m_onsetDetector=nullptr;
    setOnsetParameter("energy");
    m_numSamplesSinceLastOnset=-1;
    m_samplesSinceLastChangeOfState=0;
    is_ringing=false;
}

void NoteClassifier::setOnsetParameter(string method, float threshold,float silence,float comp,int onsetbuffersize,bool adap_whitening)
{
    m_onsetMethod=method;
    m_onsetThresh=threshold;
    m_onsetSilence=silence;
    m_onsetCompression=comp;
    m_onsetBuffersize=onsetbuffersize;
    if(m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
    m_onsetDetector=new_aubio_onset(m_onsetMethod.c_str(),m_onsetBuffersize,m_onsetBuffersize/2,m_samplerate);
    aubio_onset_set_threshold(m_onsetDetector,m_onsetThresh);
    // aubio_onset_set_awhitening(m_onsetDetector,adap_whitening);
    aubio_onset_set_silence(m_onsetDetector,m_onsetSilence);
    aubio_onset_set_compression(m_onsetDetector,m_onsetCompression);
    
    
}

void NoteClassifier::resetFilterAndOnsetDetector()
{
    if(m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
    m_onsetDetector=new_aubio_onset(m_onsetMethod.c_str(),m_onsetBuffersize,m_onsetBuffersize/2,m_samplerate);
    aubio_onset_set_threshold(m_onsetDetector,m_onsetThresh);
    // aubio_onset_set_awhitening(m_onsetDetector,adap_whitening);
    aubio_onset_set_silence(m_onsetDetector,m_onsetSilence);
    aubio_onset_set_compression(m_onsetDetector,m_onsetCompression);
#ifdef USE_ELLIPTIC
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth, m_passbandatten, 15.0);
#else
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth);
#endif
}
void NoteClassifier::setFilterParameters(float bandwidth, float passbandatten,int order)
{
    m_bandwidth=bandwidth;
    m_passbandatten=passbandatten;
    m_order=order;

    m_filter.reset();
#ifdef USE_ELLIPTIC
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth, m_passbandatten, 15.0);
#else
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth);
#endif
}

void NoteClassifier::setMidiOutput(shared_ptr<GuitarMidi::MidiOutput> output)
{
    //Set midi output
    m_midiOutput=output;
}
void NoteClassifier::initialize()
{
    //Setup FILTERORDER 1st order filters. Currently Elliptic::BandPass crashes when running setup() with orders higher than 1
    //When we solve this we can run sharper filters with narrower bandwidth and maybe drop the pitch validation below in process()
    setFilterParameters(m_bandwidth,m_passbandatten);

    //Setup a schmitt trigger as pitchdetector
    if (mPitchDetector)
        del_aubio_pitch(mPitchDetector);

    mInBufSize = mPitchBufferSize * 4;

    mPitchDetector = new_aubio_pitch("schmitt", mInBufSize, mPitchBufferSize, m_samplerate);
    if (m_pitchbuffer)
        delete[] m_pitchbuffer;
    m_pitchbuffer = new float[mInBufSize];
    m_pitchBufferCounter = 0;

    if (m_pitchfreq)
        del_fvec(m_pitchfreq);
    m_pitchfreq = new_fvec(1);

    if (m_bufferSize)
        m_buffer = new float[m_bufferSize];

    m_meanEnv=0;
    m_meanEnvCounter=0;
}

void NoteClassifier::finalize()
{
    //Release ressources
    m_filter.reset();
    if (mPitchDetector)
        del_aubio_pitch(mPitchDetector);
    if (m_pitchbuffer)
        delete[] m_pitchbuffer;
    if (m_pitchfreq)
        del_fvec(m_pitchfreq);
    if(m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
    if(m_buffer)
        delete [] m_buffer;
}

Dsp::complex_t NoteClassifier::filterResponse(float freq)
{
    return m_filter.response(freq/m_samplerate);
}

float NoteClassifier::filterAndComputeMeanEnv(const float* input,int nsamples,bool* onsetdetected)
{
//The filters work inplace so we have to initialize the output with the input data
    memcpy(m_buffer, input, nsamples * sizeof(float));
    //float* buffer=new float[nsamples];
    
    // Increase gain to increase the response in the passband
    // for (int s = 0; s < nsamples; s++)
    //     buffer[s] = 10 * buffer[s];

    // for (int s = 1; s < (nsamples-1); s++)
    //     if(fabs(buffer[s])>fabs(buffer[s-1])&&fabs(buffer[s])>fabs(buffer[s+1]))
    //     {
    //         buffer[s] *= 1.5;
    //         // buffer[s-1] *= 2;
    //         // buffer[s+2] *= 2;
    //     }
        
    m_filter.process(nsamples, &m_buffer);
    float meanenv = 0;
    int count = 0;



    if(m_onsetDetector)
    {
        fvec_t* ons=new_fvec(1);
        fvec_t onsinput;
        onsinput.data=(smpl_t*)m_buffer;
        onsinput.length=nsamples;

        aubio_onset_do(m_onsetDetector,&onsinput,ons);
        bool onsdetected=*(*ons).data>0.0;
        if(onsetdetected)
            *onsetdetected=onsdetected;
        if(onsdetected)
        {
            m_numSamplesSinceLastOnset = 0;
            m_meanEnv = 0.0;
            m_meanEnvCounter = 0;
        }
        else
            m_numSamplesSinceLastOnset+=nsamples;
        del_fvec(ons);

    }


    // if(m_meanEnvCounter>48000)
    // {
    //     m_meanEnv=0;
    //     m_meanEnvCounter=0;
    // }
    // Get average envelope

    float period=1/m_centerfreq;
    int period_samples=period*m_samplerate;
    if(m_meanEnvCounter>period_samples*4)
    {
        m_meanEnv=0;
        m_meanEnvCounter=0;
    }
    for (int s = 0; s < (nsamples); s++)
    {
         //if (fabs(buffer[s]) > fabs(buffer[s - 1]) && fabs(buffer[s]) > fabs(buffer[s + 1]) && fabs(buffer[s]) > 0)
        {
            float absval = fabs(m_buffer[s]);
            m_meanEnv += fabs(m_buffer[s]);
            // meanenv = (absval > meanenv) ? absval : meanenv;
            m_meanEnvCounter++;
        }
    }


    
    // if (count)
    //     meanenv /= count;
    //delete [] buffer;
    return m_meanEnv/m_meanEnvCounter;
}

void NoteClassifier::process(int nsamples)
{
    

    m_noteOnOffState = m_oldNoteOnOffState;
    // for (int s = 0; s < nsamples; s++)
    //     m_buffer[s] = 40 * m_buffer[s];


    float meanenv=filterAndComputeMeanEnv(input,nsamples);

    //If envelope greater then threshold consider these nsamples a candidate 
    if (meanenv > 0.002)
    {

        m_noteOnOffState = true;
        //is_ringing=true;


        

        
    }
    else
    {
        //if (meanenv < 0.05)
        {
            m_noteOnOffState = false;        // No candidate or previous note has stopped
            //m_numSamplesSinceLastOnset = -1; // No note playing
            m_meanEnv = 0.0;
            m_meanEnvCounter = 0;
            // is_ringing=false;
            // m_samplesSinceLastChangeOfState=0;
        }
    }
    setIsRinging(nsamples);
}


void NoteClassifier::setIsRinging(int nsamples)
{
    // if(m_samplesSinceLastChangeOfState>4*nsamples)
    {
        if (m_noteOnOffState != m_oldNoteOnOffState)
        {
            is_ringing = m_noteOnOffState;
            m_samplesSinceLastChangeOfState = 0;
        }
        else
            m_samplesSinceLastChangeOfState += nsamples;
        m_oldNoteOnOffState = m_noteOnOffState;
    }
}

void NoteClassifier::sendMidiNote(int nsamples, bool noteon)
{
    if (noteon)
    {
        // Send note on
        uint8_t midinote = round((log2(m_centerfreq) - log2(440)) * 12 + 69);
        uint8_t noteon[3] = {0x90, midinote, 0x7f};
        m_midiOutput->sendMidiMessage(noteon, nsamples);
    }
    else
    {
        // Send note off
        uint8_t midinote = round((log2(m_centerfreq) - log2(440)) * 12 + 69);
        uint8_t noteoff[3] = {0x90, midinote, 0x00};
        m_midiOutput->sendMidiMessage(noteoff, nsamples);
    }
}