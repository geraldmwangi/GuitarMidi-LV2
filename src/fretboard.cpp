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
FretBoard::FretBoard(LV2_URID_Map *map, float samplerate):m_noteinferencer(map)
{
    

  



}





void FretBoard::setAudioInput(const float *input)
{
    m_filterbank.setInput(input);
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
bool FretBoard::initialize(const std::string &bundle_path, float samplerate, int buffer_size)
{
        m_fretboard_rep=FretBoardRepresentation();
    m_filterbank.setup(m_fretboard_rep.get_filterrepresentations(),samplerate,buffer_size);
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
    m_filterbank.process(nsamples);
    m_noteinferencer.process(nsamples);

}
