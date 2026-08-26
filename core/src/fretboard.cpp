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
#include <guitarmidi/fretboard.hpp>
#include <omp.h>

using namespace GuitarMidi;

FretBoardAPI* creatFretBoard(MidiOutput* midioutput){
    return new FretBoard(midioutput);
}
FretBoard::FretBoard(MidiOutput* midioutput) : m_noteinferencer(midioutput)
{


}

void FretBoard::setAudioInput(const float *input)
{
    if (m_samplerate != NATIVE_SAMPLERATE)
    {
        m_input_buffer = const_cast<float *>(input);
        m_filterbank.setInput(m_resampled_buffer);
    }
    else
    {
        m_filterbank.setInput(input);
    }
}


bool FretBoard::initialize(const std::string &bundle_path, int samplerate, int buffer_size)
{
    //lv2_log_note(&g_logger, "Initializing FretBoard with samplerate %d and buffer size %d ", samplerate, buffer_size);
    g_logger.info("Initializing FretBoard with samplerate %d and buffer size %d ");
    // check if the samplerate of the host is different from the native samplerate of the plugin, if so, setup the resampler
    if (samplerate != NATIVE_SAMPLERATE)
    {
        //lv2_log_note(&g_logger, "Host samplerate %d is different from plugin samplerate %d, resampling will be performed", samplerate, NATIVE_SAMPLERATE);
        g_logger.info("Host samplerate  %d is different from plugin samplerate %d, resampling will be performed");
        m_resample_buffer_size = buffer_size;
        m_resampled_buffer = new float[m_resample_buffer_size];
        if (m_resampler.setup(samplerate, NATIVE_SAMPLERATE, 1, 16))
        {
            //lv2_log_error(&g_logger, "Failed to setup resampler");
            g_logger.error("Failed to setup resampler");
            return false;
        }
        // Initialise the resamplers for zero delay.
        m_resampler.inp_count = m_resampler.inpsize() - 1;
        m_resampler.inp_data = 0;
        m_resampler.out_count = 999999;
        m_resampler.out_data = 0;
        m_resampler.process();

        // Set the initial conditions, input buffer empty.
        m_resampler.out_data = m_resampled_buffer;
        m_resampler.out_count = buffer_size;
    }

    // setup the filterbank with the filter representations from the fretboard representation
    m_samplerate = samplerate;
    m_fretboard_rep = FretBoardRepresentation();
    m_filterbank.setup(m_fretboard_rep.get_filterrepresentations(), NATIVE_SAMPLERATE, buffer_size);
    m_noteinferencer.setAudioInputBuffer(m_filterbank.get_buffer());
    return m_noteinferencer.initialize(bundle_path);
}

void FretBoard::finalize()
{
    m_noteinferencer.finalize();

    m_filterbank.reset();
}

void FretBoard::process(int nsamples)
{
    if (m_samplerate != NATIVE_SAMPLERATE)
    {
        process_resampled(nsamples);
    }
    else
    {
        process_direct(nsamples);
    }
}



void FretBoard::process_resampled(int nsamples)
{
    m_noteinferencer.preprocess();
    m_resampler.inp_data=m_input_buffer;
    m_resampler.inp_count=nsamples;
    while(m_resampler.inp_count){
        m_resampler.process();
        if(m_resampler.out_count==0){
            m_filterbank.process(m_resample_buffer_size);
            m_noteinferencer.process(nsamples);
            m_resampler.out_data=m_resampled_buffer;
            m_resampler.out_count=m_resample_buffer_size;
        }
    }
    m_noteinferencer.postprocess();

}
void FretBoard::process_direct(int nsamples)
{
    m_filterbank.process(nsamples);

    m_noteinferencer.preprocess();
    m_noteinferencer.process(nsamples);
    m_noteinferencer.postprocess();
}