#pragma once

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

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

/**
 * @brief MidiOutput abstracts lv2's midi messaging. It needs some debugging
 * 
 */
class MidiOutput
{
private:
    /* atom-forge and URI mapping */
    LV2_Atom_Forge m_forge;
    LV2_Atom_Forge_Frame m_frame;
    LV2_URID m_midiEvent;
    LV2_Atom_Sequence *m_midioutput;

    bool forge_midimessage(
        const uint8_t *const buffer,
        uint32_t size);

public:
    MidiOutput(LV2_URID_Map *map);
    void setMidiOutput(LV2_Atom_Sequence* output);
    void sendMidiMessage(uint8_t midinote[3]);
    
};