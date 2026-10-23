// -------------------------------------------------------------------------
//
//    Copyright (C) 2010-2014 Fons Adriaensen <fons@linuxaudio.org>
//    
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// -------------------------------------------------------------------------


#ifndef __DITHER_H
#define __DITHER_H


#include <stdint.h>


class Dither
{
public:

    Dither (void);
    void reset (void);
    void proc_rectangular (int nsam, const float *srce, int16_t *dest, int ds, int dd);
    void proc_triangular  (int nsam, const float *srce, int16_t *dest, int ds, int dd);
    void proc_lipschitz   (int nsam, const float *srce, int16_t *dest, int ds, int dd); 

private:

    enum { SIZE = 64 };

    float genrand (void)
    {
        _ran *= 1103515245;
        _ran += 12345;
	return _ran / _div;
    }

    float    _err [SIZE + 4];
    int      _ind;
    uint32_t _ran;

    static float _div;
};


#endif

