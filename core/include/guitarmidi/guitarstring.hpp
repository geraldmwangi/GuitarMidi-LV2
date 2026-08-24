#pragma once
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
#include <guitarmidi/guitarnote.hpp>
using namespace std;

namespace GuitarMidi
{
    /**
     * The GuitarString class represents a single string on the guitar fretboard.
     * It contains a collection of GuitarNote objects, each representing a note that can be played on that string at different fret positions.
     */
    class GuitarString
    {
        vector<GuitarNote> m_notes;

    public:
        GuitarString(map<int,int> note_freqs);
        void get_filterrepresentations(map<uint,FilterRepresentation>& filterreps)
        {
        
            for (auto note : m_notes)
            {
                 note.get_filterrepresentations(filterreps);
                
            }
            return;
        }
    };
}
