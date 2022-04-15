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
    mBufferSize = 256;
    m_noteOnOffState = false;
}

void NoteClassifier::setMidiOutput(shared_ptr<MidiOutput> output)
{
    //Set midi output
    m_midiOutput=output;
}
void NoteClassifier::initialize()
{
    //Setup FILTERORDER 1st order filters. Currently Elliptic::BandPass crashes when running setup() with orders higher than 1
    //When we solve this we can run sharper filters with narrower bandwidth and maybe drop the pitch validation below in process()
    for (int i = 0; i < FILTERORDER; i++)
    {
        m_filter[i].reset();
        //m_filter[i].setup(MAXORDER, m_samplerate, m_centerfreq, m_bandwidth, m_passbandatten, 15.0);
        m_filter[i].setup(MAXORDER, m_samplerate, m_centerfreq, m_bandwidth);
    }

    //Setup a schmitt trigger as pitchdetector
    if (mPitchDetector)
        del_aubio_pitch(mPitchDetector);

    mInBufSize = mBufferSize * 4;

    mPitchDetector = new_aubio_pitch("schmitt", mInBufSize, mBufferSize, m_samplerate);
    if (m_pitchbuffer)
        delete[] m_pitchbuffer;
    m_pitchbuffer = new float[mInBufSize];
    m_pitchBufferCounter = 0;

    if (m_pitchfreq)
        del_fvec(m_pitchfreq);
    m_pitchfreq = new_fvec(1);
}

void NoteClassifier::finalize()
{
    //Release ressources
    for (int i = 0; i < FILTERORDER; i++)
        m_filter[i].reset();
    if (mPitchDetector)
        del_aubio_pitch(mPitchDetector);
    if (m_pitchbuffer)
        delete[] m_pitchbuffer;
    if (m_pitchfreq)
        del_fvec(m_pitchfreq);
}

void NoteClassifier::process(int nsamples)
{
    //The filters work inplace so we have to initialize the output with the input data
    memcpy(output, input, nsamples * sizeof(float));

    m_noteOnOffState = m_oldNoteOnOffState;

    //Increase gain to increase the response in the passband
    for (int s = 0; s < nsamples; s++)
        output[s] = 10 * output[s];

    for (int i = 0; i < FILTERORDER; i++)
        m_filter[i].process(nsamples, &output);

    float meanenv = 0;
    int count = 0;

    //Get average envelope
    for (int s = 1; s < (nsamples - 1); s++)
    {
        if (fabs(output[s]) > fabs(output[s - 1]) && fabs(output[s]) > fabs(output[s + 1]) && fabs(output[s]) > 0)
        {
            float absval=fabs(output[s]);
            //meanenv += fabs(output[s]);
            meanenv=(absval>meanenv)?absval:meanenv;
            count++;
        }
    }
    // if (count)
    //     meanenv /= count;

    //If envelope greater then threshold consider these nsamples a candidate 
    if (meanenv > 0.1)
    {
        memcpy(m_pitchbuffer + m_pitchBufferCounter, output, sizeof(float) * nsamples);
        m_pitchBufferCounter += nsamples;

        // Check that the pitch is correct. This step is probably unneccessary if we can increase the order of the filters see comment above in initialize()
        if (m_pitchBufferCounter >= mInBufSize)
        {
            m_pitchBufferCounter = 0;

            fvec_t Buf;
            Buf.data = m_pitchbuffer;
            Buf.length = mInBufSize;

            aubio_pitch_do(mPitchDetector, &Buf, m_pitchfreq);
            if (fabs(m_pitchfreq->data[0] - m_centerfreq) <= 4.0)
            {
                //Candidtate is valid
                m_noteOnOffState = true;
            }
            else
                m_noteOnOffState = false; //Candidtate is incorrect
        }
        //  m_noteOnOffState = true;
    }
    else
        m_noteOnOffState = false; //No candidate or previous note has stopped
    if (m_noteOnOffState != m_oldNoteOnOffState)
        if (m_noteOnOffState)
        {
            //Send note on
            uint8_t midinote = round((log2(m_centerfreq) - log2(440)) * 12 + 69);
            uint8_t noteon[3] = {0x90, midinote, 0x7f};
            m_midiOutput->sendMidiMessage(noteon,nsamples);
        }
        else
        {
            //Send note off
            uint8_t midinote = round((log2(m_centerfreq) - log2(440)) * 12 + 69);
            uint8_t noteoff[3] = {0x90, midinote, 0x00};
            m_midiOutput->sendMidiMessage(noteoff,nsamples);
        }

    m_oldNoteOnOffState = m_noteOnOffState;
}