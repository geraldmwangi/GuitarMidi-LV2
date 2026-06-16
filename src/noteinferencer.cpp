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

    NoteInferencer::NoteInferencer(LV2_URID_Map *map) : m_frames(0), m_midioutput(map)
    {
    }
    bool NoteInferencer::initialize(const std::string &bundle_path)
    {
        m_midioutput.initializeSequence();
        m_frames = 0;
        return m_model.initialize(bundle_path);
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
        m_midioutput.initializeSequence();
        stringstream msg;
        m_model.add_audio_input(m_audiobuffer.audio_buffer_2D, 1);
        float output_data[NUM_NOTES];
        if (m_model.get_model_output(output_data, 1))
        {
            msg << "Output data:";
            memcpy(m_confidences[m_current_confidence_index], output_data, sizeof(float) * NUM_NOTES);
            
            for (int i = 0; i < NUM_NOTES; i++)
            {
                //Get the per note median confidence over the last OUTWINDOW frames to reduce jitter in the output. This is a simple way to smooth the output without adding too much latency. More advanced smoothing techniques can be implemented in the future.

                std::vector<float> onset_confidences;
                int max_onset_index = *m_smoothing*OUTWINDOW; 
                max_onset_index = max(1, max_onset_index);
                for (int j = 0; j < max_onset_index; j++)
                {
                    int m=max(0, m_current_confidence_index - j);
                    onset_confidences.push_back(m_confidences[m][i]);
                }
                std::sort(onset_confidences.begin(), onset_confidences.end());
                float onset_confidence_median = onset_confidences[max_onset_index / 2];

                std::vector<float> offset_confidences;
                int max_offset_index = *m_smoothing_offset*OUTWINDOW; 
                max_offset_index = max(1, max_offset_index);
                for (int j = 0; j < max_offset_index; j++)
                {
                    int m=max(0, m_current_confidence_index - j);
                    offset_confidences.push_back(m_confidences[m][i]);
                }
                std::sort(offset_confidences.begin(), offset_confidences.end());
                float offset_confidence_median = offset_confidences[max_offset_index / 2];

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
                // smoothed_onsetoutput[i] = *m_smoothing * smoothed_onsetoutput[i] + (1 - *m_smoothing) *onset_confidence_median; // simple low pass filter to smooth the output and reduce jitter
                // smoothed_offsetoutput[i] =  *m_smoothing_offset * smoothed_offsetoutput[i] + (1 - *m_smoothing_offset) * onset_confidence_median;

                smoothed_noteenergies[i] = *m_smoothing * smoothed_noteenergies[i] + (1 - *m_smoothing) * note_energy * onset_confidence_median; // smooth the note energy to avoid jitter
                smoothed_offsetnoteenergies[i] = *m_smoothing_offset * smoothed_offsetnoteenergies[i] + (1 - *m_smoothing_offset) * note_energy * offset_confidence_median;
                if (onset_confidence_median > *m_onset_threshold)
                {

                    if (!m_note_on[i] && i != (NUM_NOTES - 1))
                    {

                        // threshold in dB is converted to linear scale by 10^(threshold_db/20)

                        float threshold = pow(10, *m_onset_energy_threshold / 20);

                        if (smoothed_noteenergies[i] < threshold)
                        {
                            #ifdef WITH_TRACING_INFO
                            lv2_log_note(&g_logger, "Note %d detected but energy %f is below threshold %f, not sending MIDI message\n", i, smoothed_noteenergies[i], threshold);
                            #endif
                            continue;
                        }
                        msg << " " << i << "(" << onset_confidence_median << ")" << " energy:" << smoothed_noteenergies[i];
                        uint8_t midinote[3] = {0x90, i + NOTE_OFFSET, 0x7f};
                        #ifdef WITH_TRACING_INFO
                        lv2_log_note(&g_logger, "Notes: %s\n", msg.str().c_str());
                        #endif

                        #ifdef WITH_AUDIO_OUTPUT
                        // visualize the detected notes in the audio output buffer by adding a sine wave at the corresponding note frequency
                        float frequency = 440.0f * pow(2.0f, (i - 9) / 12.0f); // calculate the frequency of the note
                        for (int w = 0; w < nsamples; w++)
                        {
                            audio_output[ w] = 0.1f * sin(2 * M_PI * frequency * (m_frames + w) / 48000.0f); // add a sine wave to the audio output buffer for visualization
                        }
                        #endif
                        m_midioutput.sendMidiMessage(midinote, m_frames);
                        m_note_on[i] = true;
                    }
                }
                else
                {
                    // threshold in dB is converted to linear scale by 10^(threshold_db/20)

                    float threshold = pow(10, *m_offset_energy_threshold / 20);

                    if (smoothed_offsetnoteenergies[i] > threshold)
                    {
                        #ifdef WITH_TRACING_INFO
                        lv2_log_note(&g_logger, "Note %d detected but energy %f is below threshold %f, not sending MIDI message\n", i, smoothed_offsetnoteenergies[i], threshold);
                        #endif
                        continue;
                    }
                    // lv2_log_note(&g_logger, "Note %d OFF with confidence %f\n", i, output_data[i]);
                    if (m_note_on[i] && offset_confidence_median < *m_offset_threshold && i != (NUM_NOTES - 1))
                    {
                        uint8_t midinote[3] = {0x90, i + NOTE_OFFSET, 0x00};
                        m_midioutput.sendMidiMessage(midinote, m_frames);
                        m_note_on[i] = false;
                    }
                }
            }
            m_current_confidence_index = (m_current_confidence_index + 1) % OUTWINDOW;
        }

        m_frames += nsamples;
    }
}
