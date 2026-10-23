// ----------------------------------------------------------------------------
//
//  Copyright (C) 2020 Fons Adriaensen <fons@linuxaudio.org>
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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zita-resampler/vresampler.h>


#define LINP 10000
#define LOUT 11000
#define ITER 1000


int main (int ac, char *av[])
{
    int            c, h, i;
    VResampler      R;
    float          *inp;
    float          *out;
    timespec       t0, t1;
    int64_t        ds, dn;
    double         dt;
    
    if (ac < 3)
    {
	fprintf (stderr, "vspeedtest <chan> <hlen>\n");
	return 1;
    }
    c = atoi (av [1]);
    h = atoi (av [2]);
	
    inp = new float [c * LINP];
    out = new float [c * LOUT];

    for (i = 0; i < c * LINP; i++) inp [i] = i * 1e-4f;

    R.setup (480.0 / 441.0, c, h);

    clock_gettime (CLOCK_REALTIME, &t0);
    for (i = 0; i < ITER; i++)
    {
        R.inp_count = LINP;
        R.inp_data = inp;
        R.out_count = LOUT;
        R.out_data = out;
        R.process ();
    }
    clock_gettime (CLOCK_REALTIME, &t1);
    ds = t1.tv_sec - t0.tv_sec;
    dn = t1.tv_nsec - t0.tv_nsec;
    dt = ds + 1e-9 * dn;
    printf ("44100 -> 48000, chan = %2d, hlen = %2d  %8.3le input frames per second\n",
	    c, h, LINP * ITER / dt);

    delete[] inp;
    delete[] out;
    return 0;
}

