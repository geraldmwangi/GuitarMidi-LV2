#include <noteinferencer.hpp>
#include <iostream>
#include <tensorflow/lite/logger.h>
namespace GuitarMidi
{

    // TFlite C++ API reference: https://ai.google.dev/edge/api/tflite/cc?hl=en
#define TFLITE_MINIMAL_CHECK(x)                                  \
    if (!(x))                                                    \
    {                                                            \
        fprintf(stderr, "Error at %s:%d\n", __FILE__, __LINE__); \
        exit(1);                                                 \
    }

    NoteInferencer::NoteInferencer(LV2_URID_Map *map) : m_midioutput(map)
    {
    }
    bool NoteInferencer::initialize(const std::string &bundle_path)
    {
        m_midioutput.initializeSequence();
        // #ifdef WITH_AUDIO_OUTPUT
        m_frames = 0;
        // #endif
        return m_model.initialize(bundle_path);
    }
    void NoteInferencer::finalize()
    {
        m_midioutput.finalizeSequence();
    }
    void NoteInferencer::setMidiOutput(LV2_Atom_Sequence *output)
    {
        m_midioutput.setMidiOutput(output);
    }
    void NoteInferencer::setAudioInputBuffer(AudioBuffer2D input)
    {
        m_audiobuffer = input;
    }
    void NoteInferencer::process(int nsamples)
    {
        if (m_frames < BUFFER_SIZE)
        {
            m_frames += nsamples;
            return; // only process when we have a full buffer of audio samples
        }
        int num_active_notes = 0;
        for (int h = 0; h < NUM_NOTES; h++)
        {
            num_active_notes += m_note_on[h];
        }
        m_frames = 0;
        stringstream msg;
        float gain = powf(10.0f, *m_gain_db * 0.05f);
        float expressivity = powf(10.0f, *m_expressivity_db * 0.05f);
        float en_threshold = pow(10, *m_onset_energy_threshold / 20) * gain;
        float en_threshold_offset = pow(10, *m_offset_energy_threshold / 20) * gain;
        m_model.add_audio_input(m_audiobuffer.audio_buffer_2D, 1);
        float output_data[NUM_NOTES];

        bool triggeron[NUM_NOTES] = {false};
        bool triggeroff[NUM_NOTES] = {false};
        if (m_model.get_model_output(output_data, 1))
        {
            msg << "Output data:";
            for (int i = 0; i < NUM_NOTES; i++)
            {
                // check if all harmonics of the note are active before sending note on message
                float note_energy = 0;
                int h = 1; // for (int h = 1; h <= NUM_HARMONICS; h++)
                {
                    int harmonic_index = i * NUM_HARMONICS + h - 1;
                    if (harmonic_index < NUM_HARMONICS * NUM_NOTES)
                    {
                        float harmonicenergy = 0;
                        for (int w = 0; w < BUFFER_SIZE; w++)
                        {
                            harmonicenergy += m_audiobuffer.audio_buffer_2D[harmonic_index * BUFFER_SIZE + w];
                        }
                        note_energy += harmonicenergy;
                    }
                }

                smoothed_onsetoutput[i] = *m_smoothing * smoothed_onsetoutput[i] + (1 - *m_smoothing) * output_data[i]; // simple low pass filter to smooth the output and reduce jitter
                smoothed_offsetoutput[i] = *m_smoothing_offset * smoothed_offsetoutput[i] + (1 - *m_smoothing_offset) * output_data[i];
                if (num_active_notes == 1)
                {
                    smoothed_noteenergies[i] = *m_smoothing * smoothed_noteenergies[i] + (1 - *m_smoothing) * note_energy * smoothed_onsetoutput[i]; // smooth the note energy to avoid jitter
                    smoothed_offsetnoteenergies[i] = *m_smoothing_offset * smoothed_offsetnoteenergies[i] + (1 - *m_smoothing_offset) * note_energy * smoothed_offsetoutput[i];
                }
                else
                {
                    smoothed_noteenergies[i] = *m_smoothing * smoothed_noteenergies[i] + (1 - *m_smoothing) * note_energy;                           // * smoothed_onsetoutput[i]; // smooth the note energy to avoid jitter
                    smoothed_offsetnoteenergies[i] = *m_smoothing_offset * smoothed_offsetnoteenergies[i] + (1 - *m_smoothing_offset) * note_energy; // * smoothed_offsetoutput[i];
                }


                if (!m_note_on[i] && i != (NUM_NOTES - 1))
                {
                    triggeron[i] = (smoothed_onsetoutput[i] > *m_onset_threshold && smoothed_noteenergies[i] > en_threshold);
                }
                else
                {
                    triggeroff[i] = (smoothed_offsetoutput[i] < *m_offset_threshold && smoothed_offsetnoteenergies[i] < en_threshold_offset);
                }
            }
        }
        if(num_active_notes==1)//fix for bendings in solos
            for (int i=0;i<(NUM_NOTES-1);i++)
            {
                if( smoothed_onsetoutput[i]> smoothed_onsetoutput[i+1])
                {
                    triggeroff[i+1] = m_note_on[i+1];
                }else
                {
                    triggeroff[i] = m_note_on[i];
                }
            }
        for (int i = 0; i < NUM_NOTES; i++)
        {
            if (triggeron[i])
            {
                int velocity = (int)(smoothed_noteenergies[i] / (gain * expressivity) * 127);
                velocity = std::min(velocity, 127); // cap the velocity at 127
                uint8_t midinote[3] = {0x90, i + NOTE_OFFSET, (uint8_t)velocity};
                m_midioutput.sendMidiMessage(midinote, 0);
                m_note_on[i] = true;
            }
            if (triggeroff[i])
            {
                uint8_t midinote[3] = {0x90, i + NOTE_OFFSET, 0x00};
                m_midioutput.sendMidiMessage(midinote, 0);
                m_note_on[i] = false;
            }
        }
        // #ifdef WITH_AUDIO_OUTPUT

        // #endif
    }
}
