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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <common.hpp>
#include <tensorflow/lite/version.h>
#include "tensorflow/core/public/release_version.h"
#include "tensorflow/core/public/version.h"
#include "tensorflow/lite/version.h"
#include "tensorflow/lite/core/interpreter_builder.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model_builder.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h"
//#define USE_TPU 

#ifdef USE_TPU
#include <edgetpu.h>
#endif
using namespace std;
using namespace tflite;
//#define RING_BUFFER_SIZE 8 // Number of frames to store in the ring buffer
/*
    * ModelInferencer class for handling model inference tasks. This class contains two ringbuffers for storing the last N frames of the audio input and model output.
        The audio input ringbuffer is used to store the last N frames of the audio input, which are then fed into the model for inference.
         The model output ringbuffer is used to store the last N frames of the model output, which are then used to in noteinferencer.cpp to trigger MIDI messages based on the confidence of the detected notes.
         The inferencing is performed in a separate thread to avoid blocking the audio processing thread, and the results are communicated back to the main thread using the output ringbuffer.
 */

namespace GuitarMidi{
    //spsc ring buffer for audio input and model output with atomic read and write indices for thread safety
    template<size_t NumSlots,size_t Stride>
    class SpscRingBuffer{
        float buffer[NumSlots][Stride];
        atomic<size_t> write_index;
        atomic<size_t> read_index;
    public:
        SpscRingBuffer():write_index(0),read_index(0){
           
        }
        void add_data(const float* data){
            size_t index=write_index.load();
            //performant memcpy for fixed size data
            memcpy(buffer[index&(NumSlots-1)],data,Stride*sizeof(float));
            write_index.store((index+1)); // wrap around using bitwise AND, NumSlots must be a power of 2


    }
        bool has_new_data() const{
            return write_index.load()!=read_index.load();
        }
        void get_latest_data(float* output){
            size_t index=read_index.load();
            memcpy(output,buffer[index&(NumSlots-1)],Stride*sizeof(float));
            read_index.store((index+1)); // wrap around using bitwise AND, NumSlots must be a power of 2
        }
        float* get_latest_data(){
            if(!has_new_data()){
                return nullptr;
            }
            size_t index=read_index.load();
            read_index.store((index+1));
            return buffer[index&(NumSlots-1)];
        }
    };



/*
    * ModelInferencer class for handling model inference tasks. This class contains two ringbuffers for storing the last N frames of the audio input and model output.
        The audio input ringbuffer is used to store the last N frames of the audio input, which are then fed into the model for inference.
         The model output ringbuffer is used to store the last N frames of the model output, which are then used to in noteinferencer.cpp to trigger MIDI messages based on the confidence of the detected notes.
         The inferencing is performed in a separate thread to avoid blocking the audio processing thread, and the results are communicated back to the main thread using the output ringbuffer.
*/
class ModelInferencer {
    private:
        unique_ptr<FlatBufferModel> m_model;
        unique_ptr<Interpreter> m_interpreter;
        SpscRingBuffer<RING_BUFFER_SIZE, NUM_NOTES*NUM_HARMONICS*BUFFER_SIZE> audio_input_buffer;
        SpscRingBuffer<RING_BUFFER_SIZE, NUM_NOTES> model_output_buffer;
        std::thread inferencing_thread;
        #ifdef USE_TPU
        std::shared_ptr<edgetpu::EdgeTpuContext> m_edgetpu_ctx;
        #endif
        bool stop_thread;
        void inferencing_loop();
        
    public:
        ModelInferencer();
        ~ModelInferencer();
        bool initialize(const std::string& bundle_path);
        void add_audio_input(const float* input, int num_frames);
        bool get_model_output(float* output, int num_frames);
};
} // namespace GuitarMidi
