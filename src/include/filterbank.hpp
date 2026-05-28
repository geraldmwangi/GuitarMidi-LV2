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
#include <filter.hpp>
#include <map>
#include <memory>
#include <common.hpp>
using namespace std;
#define NUM_FILTERS     (NUM_NOTES * NUM_HARMONICS)
#define NUM_SECTIONS    2      // = N (LP prototype order). BP order = 2*N = 4.
namespace GuitarMidi{

    /**
    * The FilterBank class manages a collection of Filter objects and is set up based on a provided map of filter representations.
    * It contains one 2D audio buffer that holds the output of all the filters in the bank in which each row corresponds to the output of a single filter.
    * This buffer is then used in noteinferencer to infer the played notes based on the output of the filters in the filter bank.
    *  * Difference equation direct form II implementation of the filters in the filter bank:
 * d1[n]=v[n-1]
 * d2[n]=v[n-2]
 *  v[n] =         x[n] - (a1/a0)*d1[n] - (a2/a0)*d2[n]
 *  y(n) = (b0/a0)*v[n] + (b1/a0)*d1[n] + (b2/a0)*d2[n]
    */
    class FilterBank{

        private:
            map<int,shared_ptr<Filter>> m_filters;
            AudioBuffer2D m_filterbankbuffer; //number of filters x buffersize
            //filter coefficients for the filters in the filter bank for direct form II implementation
            // Per filter, per section: biquad coefficients (a0 normalized to 1)
            float m_b0[NUM_FILTERS][NUM_SECTIONS];
            float m_b1[NUM_FILTERS][NUM_SECTIONS];
            float m_b2[NUM_FILTERS][NUM_SECTIONS];
            float m_a1[NUM_FILTERS][NUM_SECTIONS];
            float m_a2[NUM_FILTERS][NUM_SECTIONS];

            // Direct Form II state: two input history, two output history per section
            float m_s1[NUM_FILTERS][NUM_SECTIONS];
            float m_s2[NUM_FILTERS][NUM_SECTIONS];
            float *m_input;
            public:
            FilterBank(){
                // Allocate the 2D audio buffer (num_filters x window_size)
                m_filterbankbuffer.num_filters = NUM_FILTERS;
                m_filterbankbuffer.window_size = BUFFER_SIZE;
                m_filterbankbuffer.audio_buffer_2D = new float[NUM_FILTERS * m_filterbankbuffer.window_size];
            };
            ~FilterBank(){
                // Free the 2D audio buffer
                delete[] m_filterbankbuffer.audio_buffer_2D;
            };

            void setup(map<uint,FilterRepresentation> filterreps,int samplerate);

            void setInput(const float *input)
            {
                m_input = const_cast<float *>(input);
                // for(auto f:m_filters){
                //     f.second->setInput(input);
                // }
                
            }

            void process(int nsamples);

            AudioBuffer2D get_buffer(){
                return m_filterbankbuffer;
            }

            void reset();



    };
}
