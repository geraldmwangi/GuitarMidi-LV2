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

    NoteInferencer::NoteInferencer(LV2_URID_Map *map):m_frames(0),m_midioutput(map)
    {
    }
    void NoteInferencer::initialize()
    {
        m_midioutput.initializeSequence();
        m_frames=0;
        // Load model
        m_model = FlatBufferModel::BuildFromFile("/home/gerald/workspace/src/GuitarMidi-LV2/python/neuralnetmodelling/guitarmidi.tflite");
        TFLITE_MINIMAL_CHECK(m_model != nullptr);
        ops::builtin::BuiltinOpResolver resolver;
        InterpreterBuilder builder(*m_model, resolver);

        builder(&m_interpreter);
        TfLiteXNNPackDelegateOptions xnnpack_options =
            TfLiteXNNPackDelegateOptionsDefault();
        xnnpack_options.num_threads = 1; // Set number of threads as appropriate for your
                                         // platform and application needs.
        xnnpack_options.weight_cache_file_path = TfLiteXNNPackDelegateInMemoryFilePath();
        // xnnpack_options.logging_level = TFLITE_XNNPACK_LOGGING_LEVEL_ERROR;

        if (m_interpreter->ModifyGraphWithDelegate(
                TfLiteXNNPackDelegateCreate(&xnnpack_options)) != kTfLiteOk)
        {
            // Handle error, but usually optional
            fprintf(stderr, "Warning: Failed to apply XNNPACK delegate.\n");
        }
        // Allocate tensor buffers.
        TFLITE_MINIMAL_CHECK(m_interpreter->AllocateTensors() == kTfLiteOk);
        printf("=== Pre-invoke Interpreter State ===\n");
        // tflite::PrintInterpreterState(m_interpreter.get());
        tflite::LoggerOptions::SetMinimumLogSeverity(tflite::TFLITE_LOG_ERROR);
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
        TfLiteTensor *input = m_interpreter->input_tensor(0);
        TfLiteIntArray* dims = input->dims;
        msg<<"Input tensor dims:";
        for(int i=0;i<dims->size;i++){
            msg<<" "<<dims->data[i];
        }
        //printf("%s\n",msg.str().c_str());

        float* input_buffer=m_interpreter->typed_input_tensor<float>(0);
        memcpy(input_buffer,m_audiobuffer.audio_buffer_2D,m_audiobuffer.num_filters*m_audiobuffer.window_size*sizeof(float));
        TFLITE_MINIMAL_CHECK(m_interpreter->Invoke() == kTfLiteOk);

        TfLiteTensor *output = m_interpreter->output_tensor(0);
        float* output_data=m_interpreter->typed_output_tensor<float>(0);
        // print the output dims
        TfLiteIntArray* output_dims = output->dims;
        msg.str("");
        msg<<"Output tensor dims:";
        for(int i=0;i<output_dims->size;i++){
            msg<<" "<<output_dims->data[i];
        }
        //printf("%s\n",msg.str().c_str());
        int output_size=1;
        for(int i=0;i<output_dims->size;i++){
            output_size*=output_dims->data[i];
        }
        // print the output data
        msg.str("");
        msg<<"Output data:";
        for(int i=0;i<min(output_size,OUTPUT_DIM);i++){
            if(output_data[i]>0.3){
                msg<<" "<<i<<"("<<output_data[i]<<")";
                

                if(!m_note_on[i]&&i!=(OUTPUT_DIM-1)){ // avoid sending note on for the extra output used for silence detection
                    uint8_t midinote[3]={0x90,i,0x7f};
                    lv2_log_note(&g_logger, "Notes: %s\n", msg.str().c_str());
                    m_midioutput.sendMidiMessage(midinote,m_frames);
                    m_note_on[i]=true;
                }
            }
            else{
               // lv2_log_note(&g_logger, "Note %d OFF with confidence %f\n", i, output_data[i]);
                if(m_note_on[i]&&output_data[i]<0.1&&i!=(OUTPUT_DIM-1)){
                   uint8_t midinote[3]={0x90,i,0x00};
                    m_midioutput.sendMidiMessage(midinote,m_frames);
                    m_note_on[i]=false; 
                }
            }
        }

        // if(m_frames%48000==0){
        //    if(m_note_on[40]){
        //         uint8_t n=40;
        //        uint8_t midinote[3]={0x90,n,0x00};
        //        lv2_log_note(&g_logger,"Note off\n");
        //         m_midioutput.sendMidiMessage(midinote,nsamples);
        //         m_note_on[40]=false; 
        //     }
        //    else{
        //         uint8_t n=40;
        //        uint8_t midinote[3]={0x90,n,0x7f};
        //        lv2_log_note(&g_logger,"Note on\n");
        //         m_midioutput.sendMidiMessage(midinote,nsamples);
        //         m_note_on[40]=true;
        //    }
        // }

            //             uint8_t n=40;
            //    uint8_t midinote[3]={0x90,n,0x00};
            //     m_midioutput.sendMidiMessage(midinote,nsamples);
        m_frames+=nsamples;
        // printf("%s\n",msg.str().c_str());
    }
}
