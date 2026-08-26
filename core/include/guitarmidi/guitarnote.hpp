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
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <guitarmidi/common.hpp>
using namespace std;

namespace GuitarMidi
{


    /**
     * The GuitarNote class represents a note on the guitar fretboard.
     */
    class GuitarNote
    {
    private:
        vector<FilterRepresentation> m_filters;


    public:
        /** Creates a note and its harmonic filter representations. */
        GuitarNote(int note_id, float centerfreq);
        /** Releases the note resources. */
        ~GuitarNote();

        /** Adds this note's filter representations to the supplied map. */
        void get_filterrepresentations(map<uint,FilterRepresentation>& filterreps){
            for (auto f:m_filters){
                filterreps[f.filter_id]=f;
            }
        }
    };
}
