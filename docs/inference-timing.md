# Inference timing: why the queue had to change

The detector runs the neural net on a worker thread and hands results back to the
audio thread. That hand-off used an un-timestamped ring buffer that
overwrote its own unread frames, and the audio thread stamped every result with
the *current* block position. The consequence was not a rare glitch but a
systematic one: note timing depended on how fast the machine happened to run
the model, on the host block size, and on how far the worker had fallen behind.

This document records what was wrong, what the pipeline does now, and the
before/after measurements produced by `tools/latency_report`.

## What was wrong

1. **Results carried no time.** `SpscRingBuffer` stored only the payload. A
   result that surfaced two hops late was indistinguishable from a fresh one,
   and `NoteInferencer::process()` emitted it at the current block's frame
   offset. The plugin also did not expose `lv2:latency`, so the host had
   nothing to compensate.
2. **The producer ignored capacity.** `add_data()` wrote unconditionally and
   wrapped. One input frame is 148 KiB (`37 notes × 4 harmonics × 256`), so a
   wrap while the worker was reading spliced two different moments into one
   analysis window — worse than a dropped note.
3. **The worker spun.** `while (!stop) if (has_new_data()) …` burned a core and
   competed with the audio thread (and with XNNPACK's four workers).
4. **Frames depended on the host block size.** The filter bank wrote into a
   rolling 256-sample buffer and the inferencer sampled it on a `m_frames`
   accumulator, so with 100- or 300-sample blocks the model was handed a
   rotated window; a chunk crossing the row boundary also wrote into the
   neighbouring filter's row.
5. **Nothing reported a hole.** If the model fell behind, the plugin kept
   emitting stale detections and held notes that no longer matched the audio,
   with no counter to say so.

## What the pipeline does now

* **Stamped frames.** `core/include/guitarmidi/ringbuffer.hpp` is a bounded
  SPSC queue of `(stamp, frame)` pairs. `push()` refuses when full instead of
  overwriting; the payload and the stamp are published together with a release
  store and consumed with an acquire load. The capacity is set at
  instantiation from the host block size (`⌈blockSize/256⌉ +
  RING_BUFFER_SIZE` frames, rounded up to a power of two): one `process()`
  call queues a whole block of hops back to back, so a fixed 8-slot queue
  refused frames at 4096-sample blocks even with a fast model (and the MOD
  build's `RING_BUFFER_SIZE=1` would have at 1024).
* **A bounded schedule.** A result for hop *N* is emitted at the first
  completed hop boundary after the worker delivers it, so a machine that keeps
  up keeps the fast path — results are *not* held for a fixed delay. What is
  fixed is the bound: a result not in hand by its scheduled hop has missed the
  schedule and is counted as `late`. It still feeds the smoothers and may
  release notes, but it cannot start one, so a single worker stall (a
  preemption) delays the detection state by a few hops instead of cutting and
  re-triggering every held note. The
  bound is `PIPELINE_LATENCY_HOPS` hops for host blocks up to one analysis
  hop, and grows with the host block size
  (`PIPELINE_LATENCY_HOPS − 1 + ⌈blockSize/256⌉` hops), because a result can
  only be emitted at a `process()` boundary however fast the worker is:
  inside one call, consecutive hops are separated by CPU time, not audio
  time. For blocks up to 256 samples the bound —
  `(1 + PIPELINE_LATENCY_HOPS) × 256` = 768 frames ≈ 16 ms at 48 kHz — is
  reported to the host as `lv2:latency`, and it is a hard one for onsets: no
  note-on is ever emitted after the delay the host compensates.
* **Contiguous frames.** `FretBoard::process()` drives the filter bank in
  chunks that end exactly on a hop, so at every hop the rolling filter buffer
  is already linear and is queued as is (`FilterBank::analysisFrame()`);
  `captureFrame()` only linearises it for a caller that feeds unaligned
  chunks. The model therefore sees the same window for every host block size,
  and the audio thread copies each 148 KiB frame once, into the queue. The
  worker pops it straight into the interpreter's input tensor.
* **Energies travel with their result.** The worker computes the per-note
  energies from the same analysis window as the probabilities and pushes both
  through the result queue, so the energy gate is never fed a frame from a
  later hop. (The consumer used to read the *current* frame's energies
  whatever the result's stamp — a one-hop skew at small blocks, but a skew
  that varied by up to a whole host block at 1024+ and produced phantom
  on/off churn.)
* **Holes are explicit.** A gap in the result stamps (a refused frame on
  either queue, or a result lost to a pending-queue overflow) increments
  `streamGaps`. A hole of up to `PIPELINE_MAX_BRIDGED_HOPS` (8 hops, ~43 ms)
  is bridged — the one-pole smoothers just skip it. A longer one first emits
  the results from before the hole, then releases every held note, resets the
  smoothing state and increments `flushes`.
* **Deactivate/activate cycles work.** `finalize()` stops the inference
  worker but keeps the interpreter, its XNNPack delegate and the model
  loaded; `reactivate()` — wired to the LV2 `activate()` — only restarts the
  thread, with clean detection state. Hosts cycle instances around
  buffer-size changes, graph edits and latency queries (Ardour's
  `latency_compute_run()` does activate/run/deactivate for every plugin with
  a latency port), so this must be cheap: reloading the model each time took
  the GUI thread's time and leaked the delegate with its thread pool. Notes
  still held at deactivate are released at the start of the next
  `process()` call, since deactivate cannot write MIDI.
* **Failures are reported, not fatal.** A model that fails to load, build,
  delegate or allocate — or whose tensor shapes do not match the plugin —
  makes initialization return false; a failed inference skips that result
  (a hole in the stamps) and is counted in `invokeFailures`. Nothing in the
  core terminates the host process.
* **The worker sleeps** on `std::atomic::wait`, woken by the producer, and is
  stopped with an atomic flag and a notification.
* **Observability.** `getInferenceStats()` exposes hops processed, input/output
  drops, stream gaps, stale and late results, flushes, inference failures,
  note-ons and the mean/max
  inference time. The LV2 output port `latency` carries the declared delay.

### Backpressure (offline rendering)

0 ms is the real-time policy: refuse the frame and keep the audio deadline.
With that policy an offline render (a host processing as fast as it can) would
outrun the worker and lose everything, so `setBackpressureMs()` switches to a
hand-off: the producer does not let the audio run more than one hop ahead of
the results — exactly the distance a real-time run on a machine that keeps up
settles at — instead of dropping. An offline render then keeps every note,
at real-time timing. The producer never waits for a worker that is not
running (after a deactivate or a failed restart), so a render cannot stall
on it.

LV2 has a standard hook for a host to announce that it is rendering offline:
a control input with the `lv2:freeWheeling` designation, which hosts set
during export or freewheeling (Ardour and mod-host both drive it). The plugin
declares it as the optional, not-on-GUI port `lv2_freewheel`; when it goes
high the pipeline switches to the hand-off above (with a generous deadline so
a render cannot hang), and when it drops the real-time drop policy returns.
`setBackpressureMs()` remains the explicit switch for tools and tests.

## Measurements

`tools/latency_report` feeds synthetic plucks at known sample positions through
the plugin's public `FretBoardAPI` and reports, per note, the delay between the
true onset and the MIDI note-on. It only uses the interface that predates this
work, so the same source builds against the old and the new implementation and
the numbers are directly comparable. Both runs used gain 20 dB, onset smoothing
0.3, onset confidence 0.8, and a 48 kHz real-time feed. "Before" is master at
the merge base; its timing is nondeterministic by nature, so its columns move
a little between sessions (that is the bug, not measurement noise).

Mixed phrase (8 plucks, E2…E4):

| block | before: mean / max latency | before: false positives | after: mean / max latency | after: false positives |
|---|---|---|---|---|
| 64 | 29.50 / 38.67 ms | 4 | 27.65 / 35.98 ms | 1 |
| 100 | 41.67 / 72.92 ms | 11 | 27.67 / 36.00 ms | 1 |
| 256 | 35.00 / 43.33 ms | 0 | 27.65 / 35.98 ms | 1 |
| 300 | 31.12 / 42.83 ms | 4 | 27.67 / 36.00 ms | 1 |
| 480 | 34.09 / 45.33 ms | 3 | 30.34 / 41.33 ms | 1 |

Repeated pitch (8 plucks on A3; standard deviation is timing jitter, since the
pitch is constant):

| block | before: mean / sd | after: mean / sd |
|---|---|---|
| 64 | 30.34 / 1.66 ms | 26.98 / 1.53 ms |
| 100 | 39.59 / 0.01 ms | 27.00 / 1.53 ms |
| 256 | 35.00 / 2.93 ms | 26.98 / 1.53 ms |
| 300 | 30.34 / 0.01 ms | 27.67 / 1.52 ms |
| 480 | 35.34 / 0.01 ms | 31.00 / 3.61 ms |

High notes (repeated plucks on A4 and E5, blocks 64 and 256) — the case where
master's opportunistic emission was at its best and a fixed hold would have
hurt:

| build | A4 mean / max | E5 mean / max |
|---|---|---|
| before (master) | 20.34–25.67 / 22.02–30.00 ms | 19.50–24.34 / 22.02–29.33 ms |
| this branch, fixed 2-hop hold | 23.65 / 26.00 ms | 21.65 / 23.98 ms |
| this branch, emit when in hand | 18.32 / 20.67 ms | 16.32 / 18.65 ms |

Large host blocks (same mixed phrase): a `process()` call is the only
emission opportunity, so with blocks larger than an analysis hop the delay is
dominated by the block itself and the declared bound grows accordingly (1536
frames at 1024, 2560 at 2048):

| block | mean / max | false positives |
|---|---|---|
| 1024 | 39.00 / 48.00 ms | 1 |
| 2048 | 45.67 / 64.67 ms | 1 |

These sizes matter because an earlier state of this branch got them badly
wrong: a hop-counted bound that ignored the block size dropped half the
results at 1024+ (88 false positives at 1024, 63 at 2048, notes chattering as
the state was flushed every block) — which is why the bound, the pending
queue and the result payload all scale with the host block size now.

Offline (block 256, no real-time pacing):

| build | detected | mean / max latency |
|---|---|---|
| before | 6/6 | 91.67 / 122.02 ms |
| after, real-time policy | 0/6 | — (frames are refused, by design) |
| after, backpressure | 6/6 | 26.76 / 29.31 ms |

Reading the numbers:

* **The block size stops mattering.** Before, the same phrase measured 29.5 ms
  at 64 samples and up to 41.7 ms at 100 samples, with up to 11 phantom notes
  (the rotated/overlapping frames). After, every block size gives effectively
  the same mean; block 480 sits ~2.7 ms higher because its hop boundaries
  quantise event positions more coarsely, not because the pipeline behaves
  differently.
* **The fast path survives.** Emission is no longer held for a schedule: every
  note comes out several milliseconds *earlier* than before at every block
  size, and the high notes that master detected fastest (A4–E5) are detected
  faster still — 16.3 ms on E5 versus master's 19.5–24.3 ms — because the
  frames are contiguous and the results are applied as soon as they exist.
  A fixed 2-hop hold (measured above for comparison) would have cost ~5 ms on
  those notes; that is why emission follows the worker instead of the clock.
* **The delay is bounded and reported.** A result not in hand by
  `PIPELINE_LATENCY_HOPS` is counted as late and cannot start a note instead
  of silently drifting, so the declared `lv2:latency` is a hard bound for
  onsets: a note-on can never appear after the delay the host compensates.
* **Offline rendering stops lying.** Before, results were stamped with the
  block that happened to pop them, so a fast render reported "late" notes with
  a growing, unpredictable lag (91.67 ms mean here); now the drop policy is
  explicit, and backpressure restores real-time timing without the clock.
* The residual false positive (1 per phrase) is the model's harmonic double,
  which is present at every block size and is a model issue, not a timing one.

## Reproducing

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build latency_report

# Before/after comparison, from a checkout of the old and the new revision:
./build/tools/latency_report --model python/neuralnetmodelling \
    --block 300 --realtime 1 --notes 8

# Jitter for one pitch:
./build/tools/latency_report --model python/neuralnetmodelling \
    --block 300 --realtime 1 --notes 8 --pitch 50

# High notes (the fast path):
./build/tools/latency_report --model python/neuralnetmodelling \
    --block 256 --realtime 1 --notes 8 --pitch 76

# Offline render (needs backpressure to keep the notes):
./build/tools/latency_report --model python/neuralnetmodelling \
    --block 256 --realtime 0 --notes 6 --pitch 50 --backpressure 200
```

Unit and pipeline tests (CTest):

```bash
ninja -C build test_ringbuffer test_filterbank_frame test_pipeline_timing
ctest --test-dir build --output-on-failure
```

* `test_ringbuffer` — capacity, refusal, ordering, and a threaded SPSC stress
  run that checks for torn frames and stamps.
* `test_filterbank_frame` — a captured frame is contiguous and identical
  whatever chunk sizes the caller used.
* `test_pipeline_timing` — end-to-end: every block size up to 4096 detects
  the phrase, events stay inside their block and in order, latency stays
  within the declared delay plus one block, a model that keeps up never
  misses the schedule, a deliberately slow model produces drops and gaps
  without hanging notes, an offline render with backpressure keeps every
  note, the resampled 44.1 kHz path detects with a correctly scaled declared
  latency, and a deactivate/activate cycle keeps the plugin working. It also
  checks that a broken model makes initialization fail instead of exiting,
  that ten deactivate/activate cycles leave the thread count unchanged, that
  a note held at deactivate is released at the start of the next block, and
  that a 15 ms worker stall during a sustained note is counted as late
  without cutting or re-triggering the note.
* `test_lv2_wrapper` — a minimal LV2 host around the plugin's own shared
  library: real `urid:map`/`options` features, port connection by index,
  MIDI events read back out of the atom port, the latency port at 48 kHz /
  44.1 kHz / 1024-sample blocks / without options, free-wheeling renders at
  host block sizes from one sample up, the real-time drop policy when the
  free-wheeling port is absent or NULL, and a deactivate/activate cycle
  through the descriptor.
