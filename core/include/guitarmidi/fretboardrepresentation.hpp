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
#include <vector>
#include <guitarmidi/guitarstring.hpp>
using namespace std;

namespace GuitarMidi
{
/*
    The FretBoardRepresentation class represents the guitar fretboard as a collection of GuitarString objects.
    Each GuitarString contains a mapping of fret numbers to their corresponding frequencies. 
    The get_filterrepresentations method aggregates the filter representations from all strings, which can be used for further processing in the note inference model.
*/
    class FretBoardRepresentation
    {
        vector<GuitarString> m_strings;

        public:
        FretBoardRepresentation();

        map<uint,FilterRepresentation> get_filterrepresentations(){
            map<uint,FilterRepresentation> res;
            for (auto f: m_strings)
                f.get_filterrepresentations(res);

            return res;
        }
    };
}