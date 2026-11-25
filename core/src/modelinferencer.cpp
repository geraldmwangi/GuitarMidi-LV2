#include <guitarmidi/modelinferencer.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <tensorflow/lite/logger.h>

// TFlite C++ API reference: https://ai.google.dev/edge/api/tflite/cc?hl=en

namespace GuitarMidi
{
    namespace
    {
        constexpr std::size_t kInputFloats = NUM_NOTES * NUM_HARMONICS * BUFFER_SIZE;
    }

    void ModelInferencer::inferencing_loop(bool load, std::string bundle_path, const char *model_data, std::size_t model_size)
    {
        if (load)
        {
            if (!initialize_interpreter(bundle_path, model_data, model_size))
            {
                g_logger.error("Failed to initialize interpreter.\n");
                m_initialization_promise.set_value(false);
                return;
            }
            m_initialization_promise.set_value(true);
        }

        // The queue pops straight into the interpreter's input tensor: one
        // copy per frame on this side. The tensor address is stable once the
        // tensors are allocated.
        float *const input = m_interpreter->typed_input_tensor<float>(0);
        float output_frame[NUM_NOTES * 2];

        uint32_t seen = m_wake.load(std::memory_order_acquire);
        while (!m_stop.load(std::memory_order_acquire))
        {
            uint64_t stamp = 0;
            if (!m_input_queue.pop(input, stamp))
            {
                // Sleep until the audio thread queues a frame. `seen` is read
                // after the failed pop, so a frame queued in between changes
                // the counter and the wait returns immediately.
                m_wake.wait(seen, std::memory_order_acquire);
                seen = m_wake.load(std::memory_order_acquire);
                continue;
            }

            // Carry the per-note energies of the same analysis window with
            // the probabilities: the consumer's thresholds must not mix a
            // result with the energies of a later frame (the skew would
            // vary with the host block size). Computed before Invoke() so
            // nothing depends on the interpreter leaving its input intact.
            for (int i = 0; i < NUM_NOTES; ++i)
            {
                const float *row = input + i * NUM_HARMONICS * BUFFER_SIZE;
                float energy = 0.0f;
                for (int w = 0; w < BUFFER_SIZE; ++w)
                    energy += row[w];
                output_frame[NUM_NOTES + i] = energy;
            }

            const int artificial_delay = m_inference_delay_ms.load(std::memory_order_relaxed);
            const auto begin = std::chrono::steady_clock::now();
            const TfLiteStatus status = m_interpreter->Invoke();
            const auto end = std::chrono::steady_clock::now();
            if (status != kTfLiteOk)
            {
                // Skip the result: the consumer sees a hole in the stamps.
                // Log the first failure only; this repeats once per hop.
                if (m_invoke_failures.fetch_add(1, std::memory_order_relaxed) == 0)
                    g_logger.error("Model inference failed; results are being skipped\n");
                continue;
            }
            const uint64_t elapsed_us =
                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count());

            m_infer_us_sum.fetch_add(elapsed_us, std::memory_order_relaxed);
            m_infer_count.fetch_add(1, std::memory_order_relaxed);
            uint64_t previous_max = m_infer_us_max.load(std::memory_order_relaxed);
            while (elapsed_us > previous_max &&
                   !m_infer_us_max.compare_exchange_weak(previous_max, elapsed_us,
                                                         std::memory_order_relaxed))
            {
            }

            if (artificial_delay > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(artificial_delay));

            std::memcpy(output_frame, m_interpreter->typed_output_tensor<float>(0),
                        sizeof(float) * NUM_NOTES);
            // A full output queue refuses the frame rather than overwriting an
            // older result; NoteInferencer sees the resulting stamp hole.
            if (!m_output_queue.push(output_frame, stamp))
                m_output_queue.countDrop();
        }
    }

    bool ModelInferencer::initialize_interpreter(const std::string &bundle_path, const char *model_data, std::size_t model_size)
    {
        // Release in dependency order before replacing the model: the old
        // interpreter must not outlive the model or delegate it refers to.
        m_interpreter.reset();
        m_delegate.reset();
        m_model.reset();

        if (model_data != nullptr && model_size > 0)
        {
            g_logger.info("Loading model from embedded BinaryData\n");
            m_model = FlatBufferModel::BuildFromBuffer(model_data, model_size);
        }
        else
        {
            const std::string tflite_path = bundle_path + "/guitarmidi.tflite";
            g_logger.info("Loading model from: %s\n", tflite_path.c_str());
            if (!std::filesystem::exists(tflite_path))
            {
                g_logger.error("Model file not found: %s\n", tflite_path.c_str());
                return false;
            }
            m_model = FlatBufferModel::BuildFromFile(tflite_path.c_str());
        }
        if (m_model == nullptr)
        {
            g_logger.error("Failed to load the model\n");
            return false;
        }

        ops::builtin::BuiltinOpResolver resolver;
        InterpreterBuilder builder(*m_model, resolver);
        if (builder(&m_interpreter) != kTfLiteOk || m_interpreter == nullptr)
        {
            g_logger.error("Failed to build the interpreter\n");
            m_interpreter.reset();
            return false;
        }

        TfLiteXNNPackDelegateOptions xnnpack_options =
            TfLiteXNNPackDelegateOptionsDefault();
        xnnpack_options.num_threads = NUM_INFERENCE_THREADS;
        // The in-memory weight cache provider only exists in newer TensorFlow
        // Lite builds; keep the default weight cache where it is missing.
        if (TfLiteXNNPackDelegateCanUseInMemoryWeightCacheProvider())
        {
            xnnpack_options.weight_cache_file_path = TfLiteXNNPackDelegateInMemoryFilePath();
        }

        // The interpreter does not take ownership of the delegate; keep it
        // here so it (and its thread pool) is released with the interpreter.
        m_delegate.reset(TfLiteXNNPackDelegateCreate(&xnnpack_options));
        if (!m_delegate || m_interpreter->ModifyGraphWithDelegate(m_delegate.get()) != kTfLiteOk)
        {
            g_logger.error("Failed to apply XNNPack delegate\n");
            m_interpreter.reset();
            m_delegate.reset();
            return false;
        }
        if (m_interpreter->AllocateTensors() != kTfLiteOk)
        {
            g_logger.error("Failed to allocate the model tensors\n");
            m_interpreter.reset();
            m_delegate.reset();
            return false;
        }

        // The worker copies fixed-size frames in and out of the tensors; a
        // model with other shapes would be read or written out of bounds.
        const TfLiteTensor *in = m_interpreter->input_tensor(0);
        const TfLiteTensor *out = m_interpreter->output_tensor(0);
        if (in == nullptr || out == nullptr || in->type != kTfLiteFloat32 ||
            out->type != kTfLiteFloat32 || in->bytes != kInputFloats * sizeof(float) ||
            out->bytes < NUM_NOTES * sizeof(float))
        {
            g_logger.error("The model's input/output tensors do not match the plugin\n");
            m_interpreter.reset();
            m_delegate.reset();
            return false;
        }

        tflite::LoggerOptions::SetMinimumLogSeverity(tflite::TFLITE_LOG_ERROR);
        g_logger.info("Model loaded and interpreter initialized successfully.\n");
        g_logger.info("Inference thread count: %d\n", NUM_INFERENCE_THREADS);
        g_logger.info("Input queue: %zu frames\n", m_input_queue.capacity());
        return true;
    }

    bool ModelInferencer::initialize(const std::string &bundle_path)
    {
        return start_inferencing(bundle_path, nullptr, 0);
    }

    bool ModelInferencer::initializeFromBuffer(const char *model_data, std::size_t model_size)
    {
        if (model_data == nullptr || model_size == 0)
            return false;
        return start_inferencing(std::string(), model_data, model_size);
    }

    bool ModelInferencer::start_inferencing(const std::string &bundle_path, const char *model_data, std::size_t model_size)
    {
        stop_worker();

        // stop_worker() set the stop flag for the previous worker; the new
        // thread must start with it cleared or it exits immediately.
        m_stop.store(false, std::memory_order_release);
        m_initialization_promise = std::promise<bool>{};
        auto init_future = m_initialization_promise.get_future();
        m_inferencing_thread = std::thread(&ModelInferencer::inferencing_loop, this, true,
                                           bundle_path, model_data, model_size);
        if (!init_future.get())
        {
            // The thread has returned; join it so a later start is clean.
            m_inferencing_thread.join();
            g_logger.error("Failed to initialize interpreter in inferencing thread.\n");
            return false;
        }
        m_running.store(true, std::memory_order_release);
        return true;
    }

    bool ModelInferencer::restart()
    {
        if (m_inferencing_thread.joinable())
            return true;
        if (!m_interpreter)
            return false;
        m_input_queue.reset();
        m_output_queue.reset();
        m_stop.store(false, std::memory_order_release);
        m_inferencing_thread = std::thread(&ModelInferencer::inferencing_loop, this, false,
                                           std::string(), nullptr, std::size_t{0});
        m_running.store(true, std::memory_order_release);
        return true;
    }

    void ModelInferencer::stop_worker()
    {
        m_running.store(false, std::memory_order_release);
        if (!m_inferencing_thread.joinable())
            return;
        m_stop.store(true, std::memory_order_release);
        m_wake.fetch_add(1, std::memory_order_release);
        m_wake.notify_one();
        m_inferencing_thread.join();
        m_input_queue.reset();
        m_output_queue.reset();
    }

    void ModelInferencer::finalize()
    {
        stop_worker();
    }

    void ModelInferencer::setQueueCapacity(std::size_t frames)
    {
        // Only legal while stopped: resize() reallocates the slots.
        if (m_inferencing_thread.joinable())
            return;
        m_input_queue.resize(std::max<std::size_t>(frames, 1));
        // The consumer drains results at process() boundaries, so the output
        // queue must absorb a whole block's worth; output slots are small.
        m_output_queue.resize(std::max<std::size_t>(frames, 1) * 2);
    }

    ModelInferencer::ModelInferencer()
    {
    }

    ModelInferencer::~ModelInferencer()
    {
        stop_worker();
    }

    bool ModelInferencer::add_audio_input(const float *input, uint64_t stamp)
    {
        if (m_input_queue.push(input, stamp))
        {
            m_wake.fetch_add(1, std::memory_order_release);
            m_wake.notify_one();
            return true;
        }

        // Opt-in backpressure (offline rendering): wait for room instead of
        // dropping, because the host is not holding a real-time deadline.
        // Never wait for a worker that is not running: nothing would drain
        // the queue and every hop would burn the whole budget.
        const int budget_ms = m_backpressure_ms.load(std::memory_order_relaxed);
        if (budget_ms > 0 && running())
        {
            const auto deadline = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(budget_ms);
            while (std::chrono::steady_clock::now() < deadline)
            {
                std::this_thread::yield();
                if (m_input_queue.push(input, stamp))
                {
                    m_wake.fetch_add(1, std::memory_order_release);
                    m_wake.notify_one();
                    return true;
                }
            }
        }
        // Retries are not frames lost; only the final refusal is.
        m_input_queue.countDrop();
        return false;
    }

    bool ModelInferencer::get_model_output(float *output, uint64_t &stamp)
    {
        return m_output_queue.pop(output, stamp);
    }

    double ModelInferencer::meanInferenceMs() const
    {
        const uint64_t count = m_infer_count.load(std::memory_order_relaxed);
        if (count == 0)
            return 0.0;
        return static_cast<double>(m_infer_us_sum.load(std::memory_order_relaxed)) /
               static_cast<double>(count) / 1000.0;
    }

    double ModelInferencer::maxInferenceMs() const
    {
        return static_cast<double>(m_infer_us_max.load(std::memory_order_relaxed)) / 1000.0;
    }

} // namespace GuitarMidi
