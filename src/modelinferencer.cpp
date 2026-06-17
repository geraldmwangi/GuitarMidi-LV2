#include <modelinferencer.hpp>
#include <iostream>
#include <tensorflow/lite/logger.h>
#include <filesystem>
#include <chrono>
#include "tensorflow/lite/delegates/external/external_delegate.h"
// TFlite C++ API reference: https://ai.google.dev/edge/api/tflite/cc?hl=en
#define TFLITE_MINIMAL_CHECK(x)                                  \
    if (!(x))                                                    \
    {                                                            \
        fprintf(stderr, "Error at %s:%d\n", __FILE__, __LINE__); \
        exit(1);                                                 \
    }
void GuitarMidi::ModelInferencer::inferencing_loop()
{
    while (!stop_thread)
    {
        // std::unique_lock<std::mutex> lock(buffer_mutex);
        // buffer_cv.wait(lock, [this] { return stop_thread || audio_input_buffer.has_new_data(); });
        if (stop_thread)
            break;

        if (audio_input_buffer.has_new_data() && m_interpreter)
        {
            float *input_buffer = m_interpreter->typed_input_tensor<float>(0);
            audio_input_buffer.get_latest_data(input_buffer); // Assuming 1 frame of input
            #ifdef WITH_TRACING_INFO
            auto timer_start=std::chrono::high_resolution_clock::now();
            #endif
            TFLITE_MINIMAL_CHECK(m_interpreter->Invoke() == kTfLiteOk);
            #ifdef WITH_TRACING_INFO
            auto timer_end=std::chrono::high_resolution_clock::now();
            std::chrono::duration<double,std::milli> duration=timer_end-timer_start;
            stringstream msg;
            msg<<"Inference time: "<<duration.count()<<" ms"<<endl;
            lv2_log_note(&g_logger,msg.str().c_str());
            #endif

            TfLiteTensor *output = m_interpreter->output_tensor(0);
            float *output_data = m_interpreter->typed_output_tensor<float>(0);
            // print the output dims
            TfLiteIntArray *output_dims = output->dims;

            // Add the model output to the model output ring buffer
            model_output_buffer.add_data(output_data); // Assuming 1 frame of output
        }
    }
}

bool GuitarMidi::ModelInferencer::initialize(const std::string &bundle_path)
{
    // 1. Load model
    std::string tflite_path = bundle_path + "/guitarmidi.tflite";
    lv2_log_note(&g_logger, "Loading model from: %s\n", tflite_path.c_str());
    
    if (!std::filesystem::exists(tflite_path)) {
        lv2_log_error(&g_logger, "Model file not found: %s\n", tflite_path.c_str());
        return false;
    }
    
    m_model = FlatBufferModel::BuildFromFile(tflite_path.c_str());
    TFLITE_MINIMAL_CHECK(m_model != nullptr);

    // 2. Build the interpreter (DO NOT ADD EDGETPU CUSTOM OP HERE)
    ops::builtin::BuiltinOpResolverWithoutDefaultDelegates resolver;

    InterpreterBuilder builder(*m_model, resolver);
    builder.SetNumThreads(1);
    
    // Check status to prevent silent failures
    if (builder(&m_interpreter) != kTfLiteOk || !m_interpreter) {
        lv2_log_error(&g_logger, "Failed to build interpreter\n");
        return false;
    }

#ifndef USE_TPU
    TfLiteXNNPackDelegateOptions xnnpack_options = TfLiteXNNPackDelegateOptionsDefault();
    xnnpack_options.num_threads = NUM_INFERENCE_THREADS;
    xnnpack_options.weight_cache_file_path = TfLiteXNNPackDelegateInMemoryFilePath();

    auto delegate = TfLiteXNNPackDelegateCreate(&xnnpack_options);
    if (m_interpreter->ModifyGraphWithDelegate(delegate) != kTfLiteOk) {
        lv2_log_error(&g_logger, "Failed to apply XNNPack delegate\n");
        return false;
    }
#else
    // 3. Modern Edge TPU Initialization via External Delegate
    lv2_log_note(&g_logger, "Loading Edge TPU Delegate via modern external API...\n");
    
    // Loads the shared library directly. (Use "libedgetpu.so.1" on Linux)
    TfLiteExternalDelegateOptions options = TfLiteExternalDelegateOptionsDefault("/usr/lib/x86_64-linux-gnu/libedgetpu.so");
    
    // Create the delegate
    TfLiteDelegate* delegate = TfLiteExternalDelegateCreate(&options);
    if (!delegate) {
        lv2_log_error(&g_logger, "Failed to create Edge TPU delegate! Make sure libedgetpu.so.1 is installed.\n");
        return false;
    }

    // Apply the delegate (This replaces the unknown custom op automatically)
    if (m_interpreter->ModifyGraphWithDelegate(delegate) != kTfLiteOk) {
        lv2_log_error(&g_logger, "Failed to apply Edge TPU delegate to the graph\n");
        return false;
    }
    lv2_log_note(&g_logger, "Initialized TPU device and applied delegate\n");
#endif

    // 4. Allocate tensor buffers ONLY AFTER the delegate is applied
    if (m_interpreter->AllocateTensors() != kTfLiteOk) {
        lv2_log_error(&g_logger, "Failed to allocate tensors\n");
        return false;
    }

    printf("=== Pre-invoke Interpreter State ===\n");
    tflite::LoggerOptions::SetMinimumLogSeverity(tflite::TFLITE_LOG_ERROR);
    lv2_log_note(&g_logger, "Model loaded and interpreter initialized successfully.\n");
    lv2_log_note(&g_logger, "Inference thread count: %d\n", NUM_INFERENCE_THREADS);
    lv2_log_note(&g_logger, "Ring buffer size: %d frames\n", RING_BUFFER_SIZE);
    
    return true;
}

GuitarMidi::ModelInferencer::ModelInferencer()
{

    stop_thread = false;
    m_interpreter = nullptr;
    inferencing_thread = std::thread(&ModelInferencer::inferencing_loop, this);
    // log the size of the ring buffers
    lv2_log_note(&g_logger, "Audio input ring buffer size: %d frames\n", RING_BUFFER_SIZE);
    lv2_log_note(&g_logger, "Model output ring buffer size: %d frames\n", RING_BUFFER_SIZE);
}

GuitarMidi::ModelInferencer::~ModelInferencer()
{
    {

        stop_thread = true;
    }
    // buffer_cv.notify_all();
    if (inferencing_thread.joinable())
    {
        inferencing_thread.join();
    }
}

void GuitarMidi::ModelInferencer::add_audio_input(const float *input, int num_frames)
{

    audio_input_buffer.add_data(input);
}

bool GuitarMidi::ModelInferencer::get_model_output(float *output, int num_frames)
{

    if (model_output_buffer.has_new_data())
    {
        model_output_buffer.get_latest_data(output);
        return true;
    }
    return false;
}
