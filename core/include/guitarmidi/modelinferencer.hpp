#pragma once
/* GuitarMidi-LV2 Library
 * Copyright (C) 2022 Gerald Mwangi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <guitarmidi/common.hpp>
#include <guitarmidi/ringbuffer.hpp>
#include <tensorflow/lite/version.h>
#include "tensorflow/core/public/release_version.h"
#include "tensorflow/core/public/version.h"
#include <tensorflow/lite/core/interpreter_builder.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model_builder.h>
#include <tensorflow/lite/optional_debug_tools.h>
#include <tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h>

using namespace tflite;

/*
 * ModelInferencer runs the neural net on its own thread so the audio thread
 * only pays for the filter bank.
 *
 * The two threads communicate through two bounded, stamped queues: the audio
 * thread queues a completed analysis frame (148 x BUFFER_SIZE rectified filter
 * envelopes) with the index of the hop it came from, and the worker returns a
 * 37-note probability vector with that same index. Stamps are what let
 * NoteInferencer schedule results deterministically and notice a hole in the
 * stream; an un-stamped queue can only guess, which is what made note timing
 * drift under load.
 *
 * The worker sleeps on an atomic wait until a frame is queued. The previous
 * implementation spun on a flag, which burned a core and starved the audio
 * thread whenever inference became expensive.
 */
namespace GuitarMidi
{
class ModelInferencer
{
private:
    using InputQueue = SpscRingBuffer<NUM_NOTES * NUM_HARMONICS * BUFFER_SIZE>;
    /* Each output slot carries the note probabilities followed by the
     * per-note energies of the same analysis window, so the thresholds
     * always see a coherent frame. */
    using OutputQueue = SpscRingBuffer<NUM_NOTES * 2>;
    using DelegatePtr = std::unique_ptr<TfLiteDelegate, void (*)(TfLiteDelegate *)>;

    /* Declaration order is destruction order in reverse: the interpreter
     * must go before the delegate it was modified with, and both before the
     * model they were built from. */
    std::unique_ptr<FlatBufferModel> m_model;
    DelegatePtr m_delegate{nullptr, TfLiteXNNPackDelegateDelete};
    std::unique_ptr<Interpreter> m_interpreter;
    InputQueue m_input_queue{RING_BUFFER_SIZE};
    OutputQueue m_output_queue{RING_BUFFER_SIZE * 2};

    std::thread m_inferencing_thread;
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_running{false};
    std::atomic<uint32_t> m_wake{0};
    std::promise<bool> m_initialization_promise;

    std::atomic<uint64_t> m_infer_us_sum{0};
    std::atomic<uint64_t> m_infer_us_max{0};
    std::atomic<uint64_t> m_infer_count{0};
    std::atomic<uint64_t> m_invoke_failures{0};
    std::atomic<int> m_inference_delay_ms{0};
    std::atomic<int> m_backpressure_ms{0};

    /**
     * Runs model inference on frames received from the audio thread. With
     * `load` set it first builds the interpreter and reports the outcome
     * through m_initialization_promise; a restart reuses the interpreter.
     */
    void inferencing_loop(bool load, std::string bundle_path, const char *model_data, std::size_t model_size);
    /**
     * Loads the model and allocates the interpreter. A non-null `model_data`
     * takes precedence over the model file in the bundle. Never terminates
     * the process: every failure is logged and reported as false.
     */
    bool initialize_interpreter(const std::string &bundle_path, const char *model_data, std::size_t model_size);
    /** Starts the worker thread and waits for the interpreter to be ready. */
    bool start_inferencing(const std::string &bundle_path, const char *model_data, std::size_t model_size);
    /** Joins the worker thread; the interpreter stays loaded. */
    void stop_worker();

public:
    /** Creates the model inferencer. */
    ModelInferencer();
    /** Stops inference and releases model resources. */
    ~ModelInferencer();

    /** Loads the model from the plugin bundle and starts the worker. */
    bool initialize(const std::string &bundle_path);
    /** Loads the model from an embedded in-memory buffer and starts the worker. */
    bool initializeFromBuffer(const char *model_data, std::size_t model_size);
    /**
     * Stops the worker thread (LV2 deactivate). The interpreter, its
     * delegate and the model stay loaded so restart() is cheap; they are
     * released by the destructor.
     */
    void finalize();
    /**
     * Restarts the worker after finalize() with the interpreter that is
     * already loaded: no model reload, no new XNNPack thread pool. Returns
     * false if no interpreter was ever loaded; a no-op while running.
     */
    bool restart();
    /** True while the worker thread is consuming frames. */
    bool running() const { return m_running.load(std::memory_order_acquire); }

    /**
     * Sizes both queues for `frames` analysis frames in flight. One
     * process() call queues a whole host block of hops back to back, so the
     * input queue must hold at least that many plus the worker's slack, or
     * frames are refused even when inference keeps up. Allocates: only call
     * while the worker is stopped (before initialize()).
     */
    void setQueueCapacity(std::size_t frames);

    /**
     * Queue one analysis frame. `stamp` identifies the hop it belongs to.
     * Returns false if the queue was full; the result is never overwritten.
     */
    bool add_audio_input(const float *input, uint64_t stamp);
    /**
     * Pop the next result. Returns false when none is ready; on success
     * `stamp` is set to the hop the result was computed from.
     */
    bool get_model_output(float *output, uint64_t &stamp);

    uint64_t inputDropped() const { return m_input_queue.dropped(); }
    uint64_t invokeFailures() const { return m_invoke_failures.load(std::memory_order_relaxed); }
    uint64_t outputDropped() const { return m_output_queue.dropped(); }

    /** Mean/max wall time of one interpreter invocation, in milliseconds. */
    double meanInferenceMs() const;
    double maxInferenceMs() const;

    /**
     * Testing/diagnostics hook: make each inference take at least this long,
     * so the bounded-queue and staleness behaviour can be exercised without a
     * slow machine. 0 disables the delay.
     */
    void setInferenceDelayMs(int delay_ms)
    {
        m_inference_delay_ms.store(delay_ms, std::memory_order_relaxed);
    }

    /**
     * How long the producer may wait for room when the queue is full.
     *
     * 0 (default) is the real-time policy: refuse the frame, keep the audio
     * thread's deadline, let the consumer report the gap.
     *
     * A non-zero value trades the deadline for completeness, which is what an
     * offline render wants: the host is not on a clock, so waiting for
     * inference keeps every note instead of dropping the ones that arrive
     * while the model is busy.
     */
    void setBackpressureMs(int milliseconds)
    {
        m_backpressure_ms.store(milliseconds, std::memory_order_relaxed);
    }
};
} // namespace GuitarMidi
