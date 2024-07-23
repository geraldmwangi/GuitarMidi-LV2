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

    m_bufferSize = 256;
    m_buffer = nullptr;
    m_noteOnOffState = false;
    #ifdef WITH_AUBIO
    m_onsetDetector = nullptr;
    setOnsetParameter("energy");
    m_numSamplesSinceLastOnset = -1;
    #endif
    m_samplesSinceLastChangeOfState = 0;
    is_ringing = false;
}

#ifdef WITH_AUBIO
void NoteClassifier::setOnsetParameter(string method, float threshold, float silence, float comp, int onsetbuffersize, bool adap_whitening)
{
    m_onsetMethod = method;
    m_onsetThresh = threshold;
    m_onsetSilence = silence;
    m_onsetCompression = comp;
    m_onsetBuffersize = onsetbuffersize;
    if (m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
    m_onsetDetector = new_aubio_onset(m_onsetMethod.c_str(), m_onsetBuffersize, m_onsetBuffersize / 2, m_samplerate);
    aubio_onset_set_threshold(m_onsetDetector, m_onsetThresh);
    // aubio_onset_set_awhitening(m_onsetDetector,adap_whitening);
    aubio_onset_set_silence(m_onsetDetector, m_onsetSilence);
    aubio_onset_set_compression(m_onsetDetector, m_onsetCompression);
}
#endif

void NoteClassifier::resetFilterAndOnsetDetector()
{
#ifdef WITH_AUBIO
    if (m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
    m_onsetDetector = new_aubio_onset(m_onsetMethod.c_str(), m_onsetBuffersize, m_onsetBuffersize / 2, m_samplerate);
    aubio_onset_set_threshold(m_onsetDetector, m_onsetThresh);
    // aubio_onset_set_awhitening(m_onsetDetector,adap_whitening);
    aubio_onset_set_silence(m_onsetDetector, m_onsetSilence);
    aubio_onset_set_compression(m_onsetDetector, m_onsetCompression);
#endif
#ifdef USE_ELLIPTIC_FILTERS
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth, m_passbandatten, 18.0);
#else
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth);
#endif
}
void NoteClassifier::setFilterParameters(float bandwidth, float passbandatten, int order)
{
    m_bandwidth = bandwidth;
    m_passbandatten = passbandatten;
    m_order = order;

    m_filter.reset();
#ifdef USE_ELLIPTIC_FILTERS
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth, m_passbandatten, 18.0);
#else
    m_filter.setup(m_order, m_samplerate, m_centerfreq, m_bandwidth);
#endif
}

void NoteClassifier::setMidiOutput(shared_ptr<GuitarMidi::MidiOutput> output)
{
    // Set midi output
    m_midiOutput = output;
}
void NoteClassifier::initialize()
{
    // Setup FILTERORDER 1st order filters. Currently Elliptic::BandPass crashes when running setup() with orders higher than 1
    // When we solve this we can run sharper filters with narrower bandwidth and maybe drop the pitch validation below in process()
    setFilterParameters(m_bandwidth, m_passbandatten);

    if (m_bufferSize)
        m_buffer = new float[m_bufferSize];

    m_currentMeanEnv = 0;
    m_meanEnv=0;
    m_meanEnvCounter = 0;
}

void NoteClassifier::finalize()
{
    // Release ressources
    m_filter.reset();
#ifdef WITH_AUBIO
    if (m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
#endif
    if (m_buffer)
        delete[] m_buffer;
}

Dsp::complex_t NoteClassifier::filterResponse(float freq)
{
    return m_filter.response(freq / m_samplerate);
}

float NoteClassifier::filterAndComputeMeanEnv(float *buffer, int nsamples, bool *onsetdetected)
{

    // float* buffer=new float[nsamples];

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

// #ifdef WITH_TRACING_INFO
//     timespec starttime = timer_start();
// #endif
    m_filter.process(nsamples, &buffer);
// #ifdef WITH_TRACING_INFO
//     lv2_log_trace(&g_logger, "Single overtone filtering: %ld \n", timer_end(starttime));
// #endif
    float meanenv = 0;
    int count = 0;
#ifdef WITH_AUBIO
    if (m_onsetDetector)
    {
        fvec_t *ons = new_fvec(1);
        fvec_t onsinput;
        onsinput.data = (smpl_t *)buffer;
        onsinput.length = nsamples;

// #ifdef WITH_TRACING_INFO
//         starttime = timer_start();
// #endif
        aubio_onset_do(m_onsetDetector, &onsinput, ons);
// #ifdef WITH_TRACING_INFO
//         lv2_log_trace(&g_logger, "onset: %ld\n", timer_end(starttime));
// #endif

        bool onsdetected = *(*ons).data > 0.0;
        if (onsetdetected)
            *onsetdetected = onsdetected;
        if (onsdetected)
        {
            m_numSamplesSinceLastOnset = 0;
            m_meanEnv = -1.0;
            m_meanEnvCounter = 0;
        }
        else
            m_numSamplesSinceLastOnset += nsamples;
        del_fvec(ons);
    }
#endif
    // if(m_meanEnvCounter>48000)
    // {
    //     m_meanEnv=0;
    //     m_meanEnvCounter=0;
    // }
    // Get average envelope

    float period = 1 / m_centerfreq;
    int period_samples = period * m_samplerate;
    for (int s = 0; s < (nsamples); s++)
    {
        // if (fabs(buffer[s]) > fabs(buffer[s - 1]) && fabs(buffer[s]) > fabs(buffer[s + 1]) && fabs(buffer[s]) > 0)
        {
            float absval = fabs(buffer[s]);
            // m_meanEnv += fabs(buffer[s]);
            m_currentMeanEnv = (absval > m_currentMeanEnv) ? absval : m_currentMeanEnv;
            m_meanEnvCounter++;
            if (m_meanEnvCounter >= period_samples)
            {
                m_meanEnv = m_currentMeanEnv;
                // if(floor(m_centerfreq)==82)
                // {
                //     lv2_log_trace(&g_logger, "current mean env %f\n",m_meanEnv);
                // }
                m_currentMeanEnv = 0;
                m_meanEnvCounter = 0;
            }
        }
    }

    // if (count)
    //     meanenv /= count;
    // delete [] buffer;
    return m_meanEnv; // / m_meanEnvCounter;
}

void NoteClassifier::process(int nsamples)
{
    // The filters work inplace so we have to initialize the output with the input data
    memcpy(m_buffer, input, nsamples * sizeof(float));

    m_noteOnOffState = m_oldNoteOnOffState;
    // for (int s = 0; s < nsamples; s++)
    //     m_buffer[s] = 40 * m_buffer[s];

    float meanenv = filterAndComputeMeanEnv(m_buffer, nsamples);

    // If envelope greater then threshold consider these nsamples a candidate
    if (meanenv > 0.005)
    {

        m_noteOnOffState = true;
        // is_ringing=true;
    }
    else
    {
        // if (meanenv < 0.05)
        {
            m_noteOnOffState = false; // No candidate or previous note has stopped
            // m_numSamplesSinceLastOnset = -1; // No note playing
            // m_currentMeanEnv = 0.0;
            // m_meanEnvCounter = 0;
            // is_ringing=false;
            // m_samplesSinceLastChangeOfState=0;
        }
    }

    setIsRinging(nsamples);

    if(m_noteOnOffState){
        // small check
        int ring_time_max_s = 2;
        float sample_t=1.0/m_samplerate;
        float ring_time_s=sample_t*m_samplesSinceLastChangeOfState;

        // if (ring_time_s>=ring_time_max_s){
        //     lv2_log_trace(&g_logger, "WARNING Max ring time at freq: %lf. time: %lf\n ",m_centerfreq,ring_time_s);
        // }

    }

    float jitter=m_samplesSinceLastChangeOfState/nsamples;

    // if(jitter<10){
    //     lv2_log_trace(&g_logger, "WARNING JITTER at freq: %lf. jitter: %lf\n ",m_centerfreq,jitter);
    // }


}

void NoteClassifier::sendMidiNote(int nsamples)
{
    if (m_samplesSinceLastChangeOfState > 2 * nsamples)
    {
        if (m_noteOnOffState != m_oldNoteOnOffState)
            sendMidiNote(nsamples, m_noteOnOffState);

        m_oldNoteOnOffState = m_noteOnOffState;
        m_samplesSinceLastChangeOfState = 0;
    }
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