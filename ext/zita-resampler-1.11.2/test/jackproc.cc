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


// Zita-resampler demo program.
//
// Imagine you want a jack client that internally needs to use a
// fixed sample rate and period size regardless of how Jack is
// configured. This demo program contains all the buffering and
// resampling required to do this. The process in this case is
// just a copy.
//
// The signal flow is like this:
//
// jack -> queue -> resampler -> process -> resampler -> queue -> jack.
//
// The queues are required because in most cases the process period,
// taking the resampling ratio into account, will not be an integer
// fraction or multiple of jack's period. They will add some latency,
// but never more than the process period time plus the delay of the
// resamplers.


//#define USE_VRSAMPLER


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <jack/jack.h>
#include <zita-resampler/resampler.h>
#include "lfqueue.h"


#define MAXCHAN 16

static jack_client_t  *jack_handle;
static jack_port_t    *jack_capt [MAXCHAN];
static jack_port_t    *jack_play [MAXCHAN];
static bool            active = false;
static int             nchan;
static uint32_t        proc_rate;
static uint32_t        proc_frag;
static uint32_t        jack_rate;
static uint32_t        jack_frag;
#ifdef USE_VRESAMPLER
static VResampler      input_resampler;
static VResampler      output_resampler;
#else
static Resampler       input_resampler;
static Resampler       output_resampler;
#endif
static Audioqueue     *input_queue = 0;
static Audioqueue     *output_queue = 0;
static float          *input_buff = 0;
static float          *output_buff = 0;



int jack_process (jack_nframes_t nframes, void *arg)
{
    int   i, j, k, n;
    float *inp [MAXCHAN];
    float *out [MAXCHAN];
    float *p, *q;
    
    if (! active) return 0;

    // Get port buffers.
    for (i = 0; i < nchan; i++)
    {
        inp [i] = (float *)(jack_port_get_buffer (jack_capt [i], nframes));
        out [i] = (float *)(jack_port_get_buffer (jack_play [i], nframes));
    }

    // Copy from Jack ports to input queue.
    // The for loop takes care of wraparound in the queue,
    // there will be at most two iterations.
    for (n = 0; n < (int) nframes; n += k)
    {
	// Get the number of frames that can be written
	// without wraparound.
        k = input_queue->write_nowrap ();
        if (k > (int) nframes - n) k = nframes - n;
	// Copy and interleave channels.
	for (i = 0; i < nchan; i++)
	{
	    p = inp [i] + n;
	    q = input_queue->write_ptr (i);
	    for (j = 0; j < k; j++) q [nchan * j] = p [j];
	}
	// Update queue state.
	input_queue->write_commit (k);
    }
    // Check queue overflow.
    assert (input_queue->write_avail () >= 0);
    
    while (output_queue->read_avail () < (int) nframes)
    {
	// Resample from Jack's sample rate to the process
	// sample rate, reading from input_queue and writing
	// exactly proc_frag frames to input_buff.
	// The while loop takes care of wraparound.
        input_resampler.out_data = input_buff;
        input_resampler.out_count = proc_frag;
        while (input_resampler.out_count)
        {
            input_resampler.inp_data = input_queue->read_ptr ();
            input_resampler.inp_count = n = input_queue->read_nowrap ();
            input_resampler.process ();
            input_queue->read_commit (n - input_resampler.inp_count);
        }

	// Now we have proc_frag frames in input_buff.
	// Normally there would be some process using
	// these, here we just copy to output_buff.
	memcpy (output_buff, input_buff, nchan * proc_frag * sizeof (float));    
	
	// Resample from the process sample rate to Jack's
	// sample rate, taking exactly proc_frag frames
	// from output_buff, and writing to output_queue.
	// The while loop takes care of wraparound in the
	// queue.
        output_resampler.inp_data = output_buff;
        output_resampler.inp_count = proc_frag;
        while (output_resampler.inp_count)
        {
            output_resampler.out_data = output_queue->write_ptr ();
            output_resampler.out_count = n = output_queue->write_nowrap ();
            output_resampler.process ();
            output_queue->write_commit (n - output_resampler.out_count);
        }
    }
    // Check queues.
    assert (input_queue->read_avail () >= 0);
    assert (output_queue->write_avail () >= 0);

    // Copy from output queue to Jack ports.
    // The for loop takes care of wraparound in the queue,
    // there will be at most two iterations.
    for (n = 0; n < (int) nframes; n += k)
    {	   
	// Get the number of frames that can be read
	// without wraparound.
        k = output_queue->read_nowrap ();
        if (k > (int) nframes - n) k = nframes - n;
	// Copy and de-interleave channels.
	for (i = 0; i < nchan; i++)
	{
	    p = output_queue->read_ptr (i);
	    q = out [i] + n;
	    for (j = 0; j < k; j++) q [j] = p [nchan * j];
	}
	// Update queue state.
	output_queue->read_commit (k);
    }
    // Check queue underflow.
    assert (output_queue->read_avail () >= 0);
   
    return 0;
}


static void sigint_handler (int)
{
    signal (SIGINT, SIG_IGN);
    active = false;
}


int main (int ac, char *av [])
{
    int32_t        i, n;
    int64_t        k;
    char           s [16];
    jack_status_t  stat;

    if (ac < 4)
    {
	fprintf (stderr, "jackproc  <nchannel> <proc_rate> <proc_frag>\n");
	return 1;
    }
    nchan = atoi (av [1]);
    proc_rate = atoi (av [2]);
    proc_frag = atoi (av [3]);
    if (nchan < 1) return 1;
    if (nchan > MAXCHAN) nchan = MAXCHAN;

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

    for (i = 0; i < nchan; i++)
    {
	sprintf (s, "in_%d", i);
        jack_capt [i] = jack_port_register (jack_handle, s,  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	sprintf (s, "out_%d", i);
        jack_play [i] = jack_port_register (jack_handle, s, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }

    jack_rate = jack_get_sample_rate (jack_handle);
    jack_frag = jack_get_buffer_size (jack_handle);

    // Set the resampling ratios.
#ifdef USE_VRESAMPLER
    if (input_resampler.setup ((double) proc_rate / jack_rate, nchan, 32))
#else    
    if (input_resampler.setup (jack_rate, proc_rate, nchan, 32))
#endif
    {
	fprintf (stderr, "Resampler can't handle the ratio %d/%d\n",
		 proc_rate, jack_rate);
	goto cleanup;
    }
#ifdef USE_VRESAMPLER
    if (input_resampler.setup ((double) jack_rate / proc_rate, nchan, 32))
#else    
    if (output_resampler.setup (proc_rate, jack_rate, nchan, 32))
#endif
    {
	fprintf (stderr, "Resampler can't handle the ratio %d/%d\n",
		 jack_rate, proc_rate);
	goto cleanup;
    }

    // Initialise the resamplers for zero delay.
    input_resampler.inp_count = input_resampler.inpsize () - 1;
    input_resampler.inp_data = 0;
    input_resampler.out_count = 999999;
    input_resampler.out_data = 0;
    input_resampler.process ();
    output_resampler.inp_count = output_resampler.inpsize () - 1;
    output_resampler.inp_data = 0;
    output_resampler.out_count = 999999;
    output_resampler.out_data = 0;
    output_resampler.process ();
    
    input_buff = new float [nchan * proc_frag];
    output_buff = new float [nchan * proc_frag];

    // Compute the number of extra samples we need to buffer.
    k = jack_rate * proc_frag;
    n = k / proc_rate;
    
    // Create the queues, and prefill the input queue.
    input_queue = new Audioqueue (jack_frag + n, nchan, true);
    output_queue = new Audioqueue (jack_frag + n, nchan, true);
    input_queue->write_commit (n);
    
    signal (SIGINT, sigint_handler);

    // Enable the process callback and wait.
    for (active = true; active; usleep (250000));

cleanup:    
    // Cleanup.
    jack_deactivate (jack_handle);
    jack_client_close (jack_handle);
    delete[] input_buff;
    delete[] output_buff;
    delete input_queue;
    delete output_queue;

    return 0;
}

