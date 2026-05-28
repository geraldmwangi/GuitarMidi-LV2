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
            //float m_a0[NUM_NOTES*NUM_HARMONICS]={0};
            float m_a1[NUM_NOTES*NUM_HARMONICS]={0};
            float m_a2[NUM_NOTES*NUM_HARMONICS]={0};
            float m_b0[NUM_NOTES*NUM_HARMONICS]={0};
            //float m_b1[NUM_NOTES*NUM_HARMONICS]={0};
            float m_b2[NUM_NOTES*NUM_HARMONICS]={0};

            //state variables for the filters in the filter bank for direct form II implementation
            float m_d1[NUM_NOTES*NUM_HARMONICS]={0};
            float m_d2[NUM_NOTES*NUM_HARMONICS]={0};
            float m_stage2_d1[NUM_NOTES*NUM_HARMONICS]={0};
            float m_stage2_d2[NUM_NOTES*NUM_HARMONICS]={0};
            float *m_input;
            public:
            FilterBank();
            ~FilterBank();

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



    };
}
