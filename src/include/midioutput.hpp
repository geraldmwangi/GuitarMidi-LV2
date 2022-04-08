#pragma once

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

class MidiOutput
{
private:
    /* atom-forge and URI mapping */
    LV2_Atom_Forge m_forge;
    LV2_Atom_Forge_Frame m_frame;
    LV2_URID m_midiEvent;
    LV2_Atom_Sequence *m_midioutput;

    void forge_midimessage(
        const uint8_t *const buffer,
        uint32_t size);

public:
    MidiOutput(LV2_URID_Map *map);
    void setMidiOutput(LV2_Atom_Sequence* output);
    void sendMidiMessage(uint8_t midinote[3]);
    
};