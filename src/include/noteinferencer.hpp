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
#include <memory>
#include <midioutput.hpp>
#include <common.hpp>
#include <config.hpp>
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
#include <modelinferencer.hpp>
using namespace std;
using namespace tflite;
namespace GuitarMidi{
    class NoteInferencer{
       ModelInferencer m_model;
       
        GuitarMidi::MidiOutput m_midioutput;
        AudioBuffer2D m_audiobuffer;

        float* m_onset_threshold;
        float* m_offset_threshold;
        float* m_onset_energy_threshold;
        float* m_offset_energy_threshold;
        float* m_smoothing;
        float* m_smoothing_offset;
        float smoothed_onsetoutput[NUM_NOTES]={0};
        float smoothed_offsetoutput[NUM_NOTES]={0};
        float smoothed_noteenergies[NUM_NOTES]={0};
        float smoothed_offsetnoteenergies[NUM_NOTES]={0};
        // std::unique_ptr<tflite::FlatBufferModel> model;
        int64_t m_frames;
        bool m_note_on[NUM_NOTES]={false};
        public:
        NoteInferencer(LV2_URID_Map *map);

        bool initialize(const std::string& bundle_path);
        void setMidiOutput(LV2_Atom_Sequence *output);

        void setAudioInputBuffer(AudioBuffer2D input);

        void setOnsetThreshold(float* threshold){
            m_onset_threshold=threshold;
        }

        void setOffsetThreshold(float* threshold){
            m_offset_threshold=threshold;
        }

        void setSmoothing(float* smoothing){
            m_smoothing=smoothing;
        }

        void setSmoothingOffset(float* smoothing_offset){
            m_smoothing_offset=smoothing_offset;
        }

        void setOnsetEnergyThreshold(float* threshold){
            m_onset_energy_threshold=threshold;
        }
        void setOffsetEnergyThreshold(float* threshold){
            m_offset_energy_threshold=threshold;
        }
        void process(int nsamples);

#ifdef WITH_AUDIO_OUTPUT
        float *audio_output;
        void setAudioOutputBuffer(float* output){
            audio_output=output;
        }
#endif
    };
}
