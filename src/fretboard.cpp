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
#include <fretboard.hpp>
#include <omp.h>

using namespace GuitarMidi;

FretBoard::FretBoard(LV2_URID_Map *map) : m_noteinferencer(map)
{


}

void FretBoard::setAudioInput(const float *input)
{
    if (m_samplerate != SAMPLERATE)
    {
        m_input_buffer = const_cast<float *>(input);
        m_filterbank.setInput(m_resampled_buffer);
    }
    else
    {
        m_filterbank.setInput(input);
    }
}

void FretBoard::setMidiOutput(LV2_Atom_Sequence *output)
{
    m_noteinferencer.setMidiOutput(output);
    // if (m_midioutput)
    // {
    //     m_midioutput->setMidiOutput(output);
    //     m_midioutput->initializeSequence();

    //     // for (auto notecl : m_noteClassifiers)
    //     // {
    //     //     notecl->setMidiOutput(m_midioutput);
    //     // }
    // }
}
bool FretBoard::initialize(const std::string &bundle_path, int samplerate, int buffer_size)
{
    lv2_log_note(&g_logger, "Initializing FretBoard with samplerate %d and buffer size %d ", samplerate, buffer_size);
    if (samplerate != SAMPLERATE)
    {
        lv2_log_note(&g_logger, "Host samplerate %d is different from plugin samplerate %d, resampling will be performed", samplerate, SAMPLERATE);
        m_resample_buffer_size = buffer_size;
        m_resampled_buffer = new float[m_resample_buffer_size];
        if (m_resampler.setup(samplerate, SAMPLERATE, 1, 96))
        {
            lv2_log_error(&g_logger, "Failed to setup resampler");
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
    m_samplerate = samplerate;
    m_fretboard_rep = FretBoardRepresentation();
    m_filterbank.setup(m_fretboard_rep.get_filterrepresentations(), SAMPLERATE, buffer_size);
    m_noteinferencer.setAudioInputBuffer(m_filterbank.get_buffer());
    return m_noteinferencer.initialize(bundle_path);
}

void FretBoard::finalize()
{
    // for (auto notecl : m_noteClassifiers)
    // {
    //     notecl->finalize();
    // }
}

void FretBoard::process(int nsamples)
{
    if (m_samplerate != SAMPLERATE)
    {
        process_resampled(nsamples);
    }
    else
    {
        process_direct(nsamples);
    }
}

// int process (float *inp, int nframes)
// {
//     inp_resampler.inp_data = inp;
//     inp_resampler.inp_count = nframes;

//     while (inp_resampler.inp_count)
//     {
// 	inp_resampler.process ();
// 	if (inp_resampler.out_count == 0) 
// 	{
// 	    // Input buffer is full.

// //	    some_fixed_size_process (inp_buff);

// 	    inp_resampler.out_data = inp_buff;
// 	    inp_resampler.out_count = proc_frag;
// 	}
//     }
//     return 0;
// }


// int jack_process (jack_nframes_t nframes, void *arg)
// {
//     int n;
    
//     if (! active) return 0;

//     float *inp = (float *)(jack_port_get_buffer (jack_inp, nframes));

//     // Call process with a random number of frames.
//     while (nframes)
//     {
// 	n = rand () & 127;
// 	if (n > (int) nframes) n = nframes;
// 	process (inp, n);
// 	nframes -= n;
// 	inp += n;
//     }
//     return 0;
// }


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