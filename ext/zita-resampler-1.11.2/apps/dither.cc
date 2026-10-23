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


#include <string.h>
#include <math.h>
#include "dither.h"


float Dither::_div = 0;

#define SCALE 32768.0f
#define LIMIT 32767



Dither::Dither (void)
{
    reset ();
    _div = ldexpf (1.0f, 32);
}


void Dither::reset (void)
{
    memset (_err, 0, (SIZE + 4) * sizeof(float));
    _ind = SIZE - 1;
    _ran = 1234567;
}


void Dither::proc_rectangular (int nsam, const float *srce, int16_t *dest, int ds, int dd)
{
    float   v, r;
    int32_t k;

    while (nsam--)
    {
	r = genrand () - 0.5f;
        v = *srce * SCALE + r;
	k = lrintf (v);
	if      (k < -LIMIT) k = -LIMIT;
	else if (k >  LIMIT) k =  LIMIT;
        *dest = k & 0xFFFF;
        srce += ds;
        dest += dd;
    }
}


void Dither::proc_triangular (int nsam, const float *srce, int16_t *dest, int ds, int dd)
{
    float   v, r0, r1;
    int32_t k;

    r1 = *_err;
    while (nsam--)
    {
        r0 = genrand ();
        v = *srce * SCALE + r0 - r1;
	r1 = r0;
	k = lrintf (v);
	if      (k < -LIMIT) k = -LIMIT;
	else if (k >  LIMIT) k =  LIMIT;
        *dest = k & 0xFFFF;
        srce += ds;
        dest += dd;
    }
    *_err = r1;
}


void Dither::proc_lipschitz (int nsam, const float *srce, int16_t *dest, int ds, int dd)
{
    float   e, u, v, *p;
    int     i;
    int32_t k;

    i = _ind;
    while (nsam--)
    {
	p = _err + i;
        u = *srce * SCALE
	    - 2.033f * p [0]
	    + 2.165f * p [1]
	    - 1.959f * p [2]
	    + 1.590f * p [3]
	    - 0.615f * p [4];
	v = u + genrand () - genrand ();
	k = lrintf (v);
	e = k - u;
	if      (k < -LIMIT) k = -LIMIT;
	else if (k >  LIMIT) k =  LIMIT;
        *dest = k & 0xFFFF;
	if (--i < 0)
	{
	    _err [SIZE + 0] = _err [0];
	    _err [SIZE + 1] = _err [1];
	    _err [SIZE + 2] = _err [2];
	    _err [SIZE + 3] = _err [3];
	    i += SIZE;
	}
	_err [i] = e;
        srce += ds;
        dest += dd;
    }
    _ind = i;
}


