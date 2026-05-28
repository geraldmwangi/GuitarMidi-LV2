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
#include <logging.hpp>

#define BUFFER_SIZE 256
#define NOTE_OFFSET 40
#define NUM_HARMONICS 4
#define NUM_NOTES 37

#define NUM_STRINGS 6
#define NUM_FRETS 12

#define Q_FACTOR 6
namespace GuitarMidi
{

    /*
    * A 2D audio buffer structure.
        * num_filters: The number of filters used in the audio processing.
        * window_size: The size of the window used for processing the audio buffer.
        * audio_buffer_2D: A pointer to a 2D array that holds the audio data. The dimensions of this array are determined by num_filters and window_size.

    */
    struct AudioBuffer2D
    {
        int num_filters;
        int window_size;
        float *audio_buffer_2D;
    };

    /* 
    * A structure to represent the filter characteristics for a specific note in the guitar fretboard.
    * filter_id: An integer that uniquely identifies the filter associated with a particular note or its overtones.
    * center_freq: A float that represents the center frequency of the filter
    */
    struct FilterRepresentation
    {
        int filter_id;
        float center_freq;
    };
}