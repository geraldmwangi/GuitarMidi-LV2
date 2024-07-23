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
#include <midioutput.hpp>
using namespace GuitarMidi;
MidiOutput::MidiOutput(LV2_URID_Map *map)
{
    if(map)
    {
        lv2_atom_forge_init(&m_forge, map);
        m_midiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
        m_frames = 0;
    }
    m_midioutput=0;
}

bool MidiOutput::forge_midimessage(const uint8_t *const buffer,
                                   uint32_t size, int64_t frames)
{
    LV2_Atom midiatom;
    midiatom.type = m_midiEvent;
    midiatom.size = size;

    // if (0 == lv2_atom_forge_frame_time(&m_forge, frames))
    // {
    //     printf("0 == lv2_atom_forge_frame_time\n");
    //     return false;
    // }
    // if (0 == lv2_atom_forge_raw(&m_forge, &midiatom, sizeof(LV2_Atom)))
    // {
    //     printf("0==lv2_atom_forge_raw(&m_forge, &midiatom, sizeof(LV2_Atom)\n");
    //     return false;
    // }
    // if (0 == lv2_atom_forge_raw(&m_forge, buffer, size * sizeof(uint8_t)))
    // {
    //     printf("0 == lv2_atom_forge_raw(&m_forge, buffer, size*sizeof(uint8_t))\n");
    //     return false;
    // }
    // lv2_atom_forge_pad(&m_forge, size * sizeof(uint8_t) + sizeof(LV2_Atom));

    lv2_atom_forge_sequence_head(&m_forge, &m_frame, 0);
    if (0 == lv2_atom_forge_frame_time(&m_forge, 0))
    {
        printf("0 == lv2_atom_forge_frame_time\n");
        return false;
    }
    if (0 == lv2_atom_forge_atom(&m_forge, 3, m_midiEvent))
    {
        printf("0 == lv2_atom_forge_frame_time\n");
        return false;
    }

    if (0 == lv2_atom_forge_write(&m_forge, buffer, 3))
    {
        printf("0 == lv2_atom_forge_frame_time\n");
        return false;
    }

    lv2_atom_forge_pop(&m_forge, &m_frame);

    return true;
}

void MidiOutput::setMidiOutput(LV2_Atom_Sequence *output)
{
    m_midioutput = output;
    // if (m_midioutput)
    // {
    //     const uint32_t out_capacity = m_midioutput->atom.size;
    //     lv2_atom_forge_set_buffer(&m_forge, (uint8_t *)m_midioutput, out_capacity);
    //     lv2_atom_forge_sequence_head(&m_forge, &m_frame, 0);
    // }
}

void MidiOutput::initializeSequence()
{
    if (m_midioutput)
    {
        const uint32_t out_capacity = m_midioutput->atom.size;
        lv2_atom_forge_set_buffer(&m_forge, (uint8_t *)m_midioutput, out_capacity);
       // lv2_atom_forge_sequence_head(&m_forge, &m_frame, 0);
        m_frames = 0;
    }
}

void MidiOutput::sendMidiMessage(uint8_t midinote[3], int64_t frames)
{

    bool messagesent = false;
    for (int i = 0; i < 1 && !messagesent; i++)
        messagesent = forge_midimessage(midinote, 3, m_frames);
    if (!messagesent)
        printf("Failed to send midinote (%d,%d,%d)\n", midinote[0], midinote[1], midinote[2]);
    //  m_frames+=frames;
    m_frames++;
}