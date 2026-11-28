/* GuitarMidi-LV2 Library
 * Copyright (C) 2022 Gerald Mwangi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */
#include <guitarmidi/fretboard.hpp>
#include <algorithm>

using namespace GuitarMidi;

FretBoardAPI* creatFretBoard(GuitarMidiOutput* midioutput){
    return new FretBoard(midioutput);
}
FretBoard::FretBoard(GuitarMidiOutput* midioutput) : m_noteinferencer(midioutput)
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
    if (!initializeFilterbank(samplerate, buffer_size))
        return false;
    return m_noteinferencer.initialize(bundle_path);
}

bool FretBoard::initializeFromBuffer(const char* model_data, std::size_t model_size, int samplerate, int buffer_size)
{
    if (!initializeFilterbank(samplerate, buffer_size))
        return false;
    return m_noteinferencer.initializeFromBuffer(model_data, model_size);
}

bool FretBoard::initializeFilterbank(int samplerate, int buffer_size)
{
    //lv2_log_note(&g_logger, "Initializing FretBoard with samplerate %d and buffer size %d ", samplerate, buffer_size);
    g_logger.info("Initializing FretBoard with samplerate %d and buffer size %d ", samplerate, buffer_size);
    // check if the samplerate of the host is different from the native samplerate of the plugin, if so, setup the resampler
    if (samplerate != NATIVE_SAMPLERATE)
    {
        g_logger.info("Host samplerate  %d is different from plugin samplerate %d, resampling will be performed", samplerate, NATIVE_SAMPLERATE);
        m_resample_buffer_size = buffer_size;
        m_resampled_buffer = new float[m_resample_buffer_size];
        if (m_resampler.setup(samplerate, NATIVE_SAMPLERATE, 1, 16))
        {
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
    // The scheduling bound must know the host block size: with blocks larger
    // than an analysis hop, a result can only be emitted at the next
    // process() call however fast the worker is.
    m_noteinferencer.setHostBufferSize(buffer_size);
    m_fretboard_rep = FretBoardRepresentation();
    m_filterbank.setup(m_fretboard_rep.get_filterrepresentations(), NATIVE_SAMPLERATE, buffer_size);
    return true;
}

void FretBoard::finalize()
{
    m_noteinferencer.finalize();

    m_filterbank.reset();
    m_hopFill = 0;
}

bool FretBoard::reactivate()
{
    // The filter bank state was reset by finalize(); the worker restart
    // also starts detection from a clean slate, so a deactivate/activate
    // cycle behaves like the start of a session.
    return m_noteinferencer.reactivate();
}

void FretBoard::process(int nsamples)
{
    // prepare the output stream
    m_noteinferencer.preprocess();
    m_blockSize = nsamples;

    // Drive the filter bank in chunks that end exactly on an analysis hop.
    // That keeps the rolling filter buffer aligned (the model is never handed
    // a rotated window) and gives every hop a real position in the block.
    int done = 0;
    while (done < nsamples)
    {
        if (m_samplerate != NATIVE_SAMPLERATE)
        {
            int n = nsamples - done;
            const int budget = m_resampler.inpsize() - 1;
            if (budget > 0 && n > budget)
                n = budget;
            process_resampled(n, done);
            done += n;
        }
        else
        {
            const int n = std::min(nsamples - done, BUFFER_SIZE - m_hopFill);
            process_direct(n, done);
            done += n;
        }
    }

    m_noteinferencer.postprocess();
}

void FretBoard::process_resampled(int nsamples,int pos)
{
    m_resampler.inp_data = m_input_buffer + pos;
    m_resampler.inp_count = nsamples;
    while (m_resampler.inp_count > 0)
    {
        m_resampler.out_data = m_resampled_buffer;
        m_resampler.out_count = m_resample_buffer_size;
        const int before_in = m_resampler.inp_count;
        m_resampler.process();
        const int produced = m_resample_buffer_size - m_resampler.out_count;
        const int consumed = before_in - m_resampler.inp_count;
        if (produced > 0)
        {
            // The hop completed somewhere inside this input chunk; place its
            // events there instead of at the block boundary.
            const int eventPos = pos + std::min(nsamples - 1, consumed);
            consumeResampled(m_resampled_buffer, produced, eventPos, pos);
        }
        if (consumed == 0 && produced == 0)
            break; // the resampler made no progress; do not spin
    }
}

void FretBoard::consumeResampled(const float* data,int count,int eventPos,int hostPos)
{
    int off = 0;
    while (off < count)
    {
        const int n = std::min(count - off, BUFFER_SIZE - m_hopFill);
        // The filter output is indexed in host-block frames: resampled
        // frame j is j * m_samplerate / NATIVE_SAMPLERATE host frames into
        // the chunk that started at hostPos. Clamped to the block, so the
        // visualization can never write past the port buffer.
        const int64_t outPos = std::min<int64_t>(
            m_blockSize - 1,
            hostPos + (static_cast<int64_t>(off) * m_samplerate) / NATIVE_SAMPLERATE);
        m_filterbank.process(n, off, static_cast<int>(outPos));
        off += n;
        m_hopFill += n;
        if (m_hopFill == BUFFER_SIZE)
        {
            m_hopFill = 0;
            m_noteinferencer.processHop(m_filterbank.analysisFrame(),
                                        std::min<int64_t>(m_blockSize - 1, eventPos));
        }
    }
}

void FretBoard::process_direct(int nsamples,int pos)
{
    m_filterbank.process(nsamples,pos);
    m_hopFill += nsamples;
    if (m_hopFill >= BUFFER_SIZE)
    {
        m_hopFill = 0;
        // pos + nsamples is the first sample after the analysed window; clamp
        // to the block because an LV2 event time must be inside it.
        m_noteinferencer.processHop(m_filterbank.analysisFrame(),
                                    std::min<int64_t>(m_blockSize - 1, pos + nsamples));
    }
}

void FretBoard::getFilterWaveforms(float *waveforms, uint32_t samples_per_filter) const
{
    if (samples_per_filter == 0)
        return;

    const AudioBuffer2D filter_buffer = m_filterbank.get_waveform_buffer();
    for (int filter = 0; filter < NUM_FILTERS; ++filter)
    {
        const float *samples = filter_buffer.audio_buffer_2D + filter * filter_buffer.window_size;
        for (uint32_t point = 0; point < samples_per_filter; ++point)
        {
            const uint32_t sample = point * filter_buffer.window_size / samples_per_filter;
            waveforms[filter * samples_per_filter + point] = samples[sample];
        }
    }
}
