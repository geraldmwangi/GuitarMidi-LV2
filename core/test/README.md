# gmtest — core processing harness

`gmtest` is a small headless harness that drives `FretBoard` directly, without any
plugin host. It feeds a synthetic sine wave through the full pipeline
(filterbank -> model inference -> MIDI output stub) and reports any failures.
It is the quickest way to exercise the core across buffer sizes and samplerates,
and it works under sanitizers (AddressSanitizer) since the plugin host does not have to be involved.

## Building

The harness is off by default. Enable it with the `GUITARMIDI_BUILD_TEST_HARNESS` option:

```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGUITARMIDI_BUILD_TEST_HARNESS=ON ..
cmake --build . --target gmtest
```

The binary lands at `ext/GuitarMidi-LV2/core/gmtest` (relative to the build directory).
In the ArtiString repo the presets already enable it for `linux-debug` and `linux-asan`:

```bash
cmake --build --preset=linux-asan --target gmtest
```

## Usage

```
gmtest [block] [bundle_dir] [samplerate]
gmtest -m [bundle_dir]     # run the rate/block matrix
```

* `block` — host block size in frames (default 256)
* `bundle_dir` — model bundle directory containing `guitarmidi.tflite` (default `ext/GuitarMidi-LV2/python/neuralnetmodelling`)
* `samplerate` — pretend samplerate for `initialize()`/`process()` (default 48000; other rates go through the resampling path)

Single run from the ArtiString repo root:

```bash
ASAN_OPTIONS=detect_leaks=0 ./build/linux-asan/ext/GuitarMidi-LV2/core/gmtest 4096 ext/GuitarMidi-LV2/python/neuralnetmodelling 44100
```

Matrix mode runs 6 samplerates (22050, 32000, 44100, 48000, 88200, 96000) x
5 block sizes (100, 256, 1024, 2048, 4096) and prints one `OK` line per combo plus a
`PASS`/`FAIL` summary; exit code is 0 when all combos pass, 1 when any fail:

```bash
ASAN_OPTIONS=detect_leaks=0 ./build/linux-asan/ext/GuitarMidi-LV2/core/gmtest -m
```

## AddressSanitizer

With `ARTISTRING_ASAN=ON` the core is compiled with `-fsanitize=address` and any
target linking `guitarmidi_core` (including `gmtest`) automatically links libasan,
so the harness catches memory errors with full symbolized stacks while the plugin
itself stays uninstrumented. Run with `ASAN_OPTIONS=detect_leaks=0` — the core
still has intentional long-lived allocations (model, interpreter, ring buffers).
