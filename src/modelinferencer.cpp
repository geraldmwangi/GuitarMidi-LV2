#include <modelinferencer.hpp>
#include <iostream>
#include <tensorflow/lite/logger.h>
    // TFlite C++ API reference: https://ai.google.dev/edge/api/tflite/cc?hl=en
#define TFLITE_MINIMAL_CHECK(x)                                  \
    if (!(x))                                                    \
    {                                                            \
        fprintf(stderr, "Error at %s:%d\n", __FILE__, __LINE__); \
        exit(1);                                                 \
    }
void GuitarMidi::ModelInferencer::inferencing_loop()
{
    while (!stop_thread) {
        // std::unique_lock<std::mutex> lock(buffer_mutex);
        // buffer_cv.wait(lock, [this] { return stop_thread || audio_input_buffer.has_new_data(); });
        if (stop_thread) break;

      

    if (audio_input_buffer.has_new_data() && m_interpreter) {
            float* input_buffer=m_interpreter->typed_input_tensor<float>(0);
            audio_input_buffer.get_latest_data(input_buffer); // Assuming 1 frame of input
            TFLITE_MINIMAL_CHECK(m_interpreter->Invoke() == kTfLiteOk);

            TfLiteTensor *output = m_interpreter->output_tensor(0);
            float* output_data=m_interpreter->typed_output_tensor<float>(0);
            // print the output dims
            TfLiteIntArray* output_dims = output->dims;
    
                // Add the model output to the model output ring buffer
                model_output_buffer.add_data(output_data); // Assuming 1 frame of output
            }


    }
}

void GuitarMidi::ModelInferencer::initialize()
{
            // Load model
        m_model = FlatBufferModel::BuildFromFile("/usr/lib/lv2/guitarmidi.lv2/guitarmidi.tflite");
        TFLITE_MINIMAL_CHECK(m_model != nullptr);
        ops::builtin::BuiltinOpResolver resolver;
        InterpreterBuilder builder(*m_model, resolver);

        builder(&m_interpreter);
        TfLiteXNNPackDelegateOptions xnnpack_options =
            TfLiteXNNPackDelegateOptionsDefault();
        xnnpack_options.num_threads = 4; // Set number of threads as appropriate for your
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

GuitarMidi::ModelInferencer::ModelInferencer()
{

    stop_thread = false;
    m_interpreter=nullptr;
    inferencing_thread = std::thread(&ModelInferencer::inferencing_loop, this);
}

GuitarMidi::ModelInferencer::~ModelInferencer()
{
    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        stop_thread = true;
    }
    // buffer_cv.notify_all();
    if (inferencing_thread.joinable()) {
        inferencing_thread.join();
    }
}

void GuitarMidi::ModelInferencer::add_audio_input(const float *input, int num_frames)
{
    
    audio_input_buffer.add_data(input);

}

bool GuitarMidi::ModelInferencer::get_model_output(float *output, int num_frames)
{
    
    if (model_output_buffer.has_new_data()) {
        model_output_buffer.get_latest_data(output);
        return true;
    }
    return false;
}
