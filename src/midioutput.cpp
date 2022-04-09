#include <midioutput.hpp>

MidiOutput::MidiOutput(LV2_URID_Map *map)
{
    lv2_atom_forge_init(&m_forge, map);
    m_midiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
}

bool MidiOutput::forge_midimessage(const uint8_t *const buffer,
                                   uint32_t size)
{
    LV2_Atom midiatom;
    midiatom.type = m_midiEvent;
    midiatom.size = size;

    if (0 == lv2_atom_forge_frame_time(&m_forge, 0))
        return false;
    if (0 == lv2_atom_forge_raw(&m_forge, &midiatom, sizeof(LV2_Atom)))
        return false;
    if (0 == lv2_atom_forge_raw(&m_forge, buffer, size))
        return false;
    lv2_atom_forge_pad(&m_forge, sizeof(LV2_Atom) + size);
    return true;
}

void MidiOutput::setMidiOutput(LV2_Atom_Sequence *output)
{
    m_midioutput = output;
}

void MidiOutput::sendMidiMessage(uint8_t midinote[3])
{
    const uint32_t out_capacity = m_midioutput->atom.size;
    lv2_atom_forge_set_buffer(&m_forge, (uint8_t *)m_midioutput, out_capacity);
    lv2_atom_forge_sequence_head(&m_forge, &m_frame, 0);

    bool messagesent=false;
    for(int i=0;i<3&&!messagesent;i++)
        messagesent=forge_midimessage(midinote, 3);
    if(!messagesent)
        printf("Failed to send midinote (%d,%d,%d)\n",midinote[0],midinote[1],midinote[2]);
}