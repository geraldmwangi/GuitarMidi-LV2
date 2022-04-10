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

MidiOutput::MidiOutput(LV2_URID_Map *map)
{
    lv2_atom_forge_init(&m_forge, map);
    m_midiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
    m_frames=0;
}

bool MidiOutput::forge_midimessage(const uint8_t *const buffer,
                                   uint32_t size,int64_t frames)
{
    LV2_Atom midiatom;
    midiatom.type = m_midiEvent;
    midiatom.size = size;

    if (0 == lv2_atom_forge_frame_time(&m_forge, frames))
        return false;
    if (0 == lv2_atom_forge_raw(&m_forge, &midiatom, sizeof(LV2_Atom)))
        return false;
    if (0 == lv2_atom_forge_raw(&m_forge, buffer, size))
        return false;
    lv2_atom_forge_pad(&m_forge, 8- size);
    return true;
}

void MidiOutput::setMidiOutput(LV2_Atom_Sequence *output)
{
    m_midioutput = output;
}

void MidiOutput::sendMidiMessage(uint8_t midinote[3],int64_t frames)
{
    const uint32_t out_capacity = m_midioutput->atom.size;
    lv2_atom_forge_set_buffer(&m_forge, (uint8_t *)m_midioutput, out_capacity);
    lv2_atom_forge_sequence_head(&m_forge, &m_frame, 0);

    bool messagesent=false;
    for(int i=0;i<1&&!messagesent;i++)
        messagesent=forge_midimessage(midinote, 3,m_frames);
    if(!messagesent)
        printf("Failed to send midinote (%d,%d,%d)\n",midinote[0],midinote[1],midinote[2]);
    // m_frames+=frames;
}