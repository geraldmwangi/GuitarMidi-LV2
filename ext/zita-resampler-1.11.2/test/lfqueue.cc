// ----------------------------------------------------------------------------
//
//  Copyright (C) 2012-2016 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#include "lfqueue.h"


Queuebase::Queuebase (int nelm) :
    _nelm (0),
    _mask (0),
    _kwrite (0),
    _kread (0)
{
    int32_t k;

    if (nelm > 0x01000000) return;
    for (k = 1; k < nelm; k <<= 1);
    _nelm = k;
    _mask = k - 1;
}


Queuebase::~Queuebase (void)
{
}   



Audioqueue::Audioqueue (int32_t minsize, int nchannel, bool interleaved) :
    Queuebase (minsize),
    _nchan (nchannel),
    _inter (interleaved),
    _data (0)
{
    _data = new float [_nelm * _nchan];
}


Audioqueue::~Audioqueue (void)
{
    delete[] _data;
}   


