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
#include <guitarmidi/fretboard_api.hpp>
#include <guitarmidi/common.hpp>
#include <guitarmidi/config.hpp>
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
#include <guitarmidi/modelinferencer.hpp>
using namespace std;
using namespace tflite;
namespace GuitarMidi{
    /**
     * Converts model note predictions into MIDI events while applying the configured thresholds and smoothing.
     */
    class NoteInferencer{
       ModelInferencer m_model;
       
        GuitarMidiOutput* m_midioutput=nullptr;
        AudioBuffer2D m_audiobuffer;
        float* m_gain_db;
        float* m_expressivity_db;
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
        //#ifdef WITH_AUDIO_OUTPUT
        int64_t m_frames=0;
        //#endif
        bool m_note_on[NUM_NOTES]={false};
        public:
        /** Creates a note inferencer connected to the supplied MIDI output. */
        NoteInferencer(GuitarMidiOutput *midioutput);
        /** Releases the MIDI output owned by the inferencer. */
        ~NoteInferencer(){
            if(m_midioutput)
                delete m_midioutput;
        }

        /** Loads and initializes the neural-network model. */
        bool initialize(const std::string& bundle_path);
        /** Finalizes model inference and releases inference resources. */
        void finalize();
        /** Returns the MIDI output used for inferred notes. */
        GuitarMidiOutput* getMidiOutput()
        {
            return m_midioutput;
        }
        /** Sets the filter-bank buffer consumed by inference. */
        void setAudioInputBuffer(AudioBuffer2D input);

        /** Sets the gain control. */
        void setGain(float* gain){
            m_gain_db=gain;
        }
        /** Sets the MIDI expressivity control. */
        void setExpressivity(float* expressivity){
            m_expressivity_db=expressivity;
        }
        /** Sets the note-on threshold control. */
        void setOnsetThreshold(float* threshold){
            m_onset_threshold=threshold;
        }

        /** Sets the note-off threshold control. */
        void setOffsetThreshold(float* threshold){
            m_offset_threshold=threshold;
        }

        /** Sets the onset smoothing control. */
        void setSmoothing(float* smoothing){
            m_smoothing=smoothing;
        }

        /** Sets the offset smoothing control. */
        void setSmoothingOffset(float* smoothing_offset){
            m_smoothing_offset=smoothing_offset;
        }

        /** Sets the note-on energy threshold control. */
        void setOnsetEnergyThreshold(float* threshold){
            m_onset_energy_threshold=threshold;
        }
        /** Sets the note-off energy threshold control. */
        void setOffsetEnergyThreshold(float* threshold){
            m_offset_energy_threshold=threshold;
        }

        /** Starts a MIDI output sequence before processing a block. */
        void preprocess(){
            if(m_midioutput)
                m_midioutput->initializeSequence();
        }

        /** Completes a MIDI output sequence after processing a block. */
        void postprocess(){
            if(m_midioutput)
                m_midioutput->finalizeSequence();
        }
        /** Converts model predictions for an audio block into MIDI events. */
        void process(int nsamples);

#ifdef WITH_AUDIO_OUTPUT
        float *audio_output;
        /** Sets the optional audio output buffer. */
        void setAudioOutputBuffer(float* output){
            audio_output=output;
        }
#endif
    };
}
