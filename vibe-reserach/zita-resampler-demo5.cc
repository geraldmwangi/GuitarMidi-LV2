// -----------------------------------------------------------------------------
//
//  Copyright (C) 2020 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
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
// -----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <jack/jack.h>
#include <zita-resampler/resampler.h>


static jack_client_t  *jack_handle;
static jack_port_t    *jack_inp;
static bool            active = false;
static uint32_t        proc_rate;
static uint32_t        proc_frag;
static uint32_t        jack_rate;
static Resampler       inp_resampler;
static float          *inp_buff = 0;


int process (float *inp, int nframes)
{
    inp_resampler.inp_data = inp;
    inp_resampler.inp_count = nframes;

    while (inp_resampler.inp_count)
    {
	inp_resampler.process ();
	if (inp_resampler.out_count == 0) 
	{
	    // Input buffer is full.

//	    some_fixed_size_process (inp_buff);

	    inp_resampler.out_data = inp_buff;
	    inp_resampler.out_count = proc_frag;
	}
    }
    return 0;
}


int jack_process (jack_nframes_t nframes, void *arg)
{
    int n;
    
    if (! active) return 0;

    float *inp = (float *)(jack_port_get_buffer (jack_inp, nframes));

    // Call process with a random number of frames.
    while (nframes)
    {
	n = rand () & 127;
	if (n > (int) nframes) n = nframes;
	process (inp, n);
	nframes -= n;
	inp += n;
    }
    return 0;
}


static void sigint_handler (int)
{
    signal (SIGINT, SIG_IGN);
    active = false;
}


int main (int ac, char *av [])
{
    jack_status_t  stat;

    if (ac < 3)
    {
	fprintf (stderr, "jackproc <proc_rate> <proc_frag>\n");
	return 1;
    }
    proc_rate = atoi (av [1]);  // Process sample rate
    proc_frag = atoi (av [2]);  // Process buffer size

    // Create and initialise the Jack client.
    jack_handle = jack_client_open ("Jackproc", JackNoStartServer, &stat);
    if (jack_handle == 0)
    {
        fprintf (stderr, "Can't connect to Jack, is the server running ?\n");
        return 1;
    }

    jack_set_process_callback (jack_handle, jack_process, 0);
    if (jack_activate (jack_handle))
    {
        fprintf(stderr, "Can't activate Jack");
        return 1;
    }

    // Create ports.
    jack_inp = jack_port_register (jack_handle, "inp", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    // Set the resampling ratios.
    jack_rate = jack_get_sample_rate (jack_handle);
    if (inp_resampler.setup (jack_rate, proc_rate, 1, 32))
    {
	fprintf (stderr, "Resampler can't handle the ratio %d/%d\n",
		 proc_rate, jack_rate);
	goto cleanup;
    }

    inp_buff = new float [proc_frag];

    // Initialise the resamplers for zero delay.
    inp_resampler.inp_count = inp_resampler.inpsize () - 1;
    inp_resampler.inp_data = 0;
    inp_resampler.out_count = 999999;
    inp_resampler.out_data = 0;
    inp_resampler.process ();

    // Set the initial conditions, input buffer empty.
    inp_resampler.out_data = inp_buff;
    inp_resampler.out_count = proc_frag;
    
    signal (SIGINT, sigint_handler);

    // Enable processing and wait.
    for (active = true; active; usleep (250000));

cleanup:    
    jack_deactivate (jack_handle);
    jack_client_close (jack_handle);
    delete[] inp_buff;

    return 0;
}

