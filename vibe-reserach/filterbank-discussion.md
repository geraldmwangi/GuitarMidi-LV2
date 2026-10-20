how does my c++ filterbank compare to the wavelet approach. can wavelet filterbanks denser features at the same computational and group latency?

## Short answer

Your bank is **already a wavelet-style (constant‑Q) filterbank** in every way that matters for latency: `bw = fc / Q_FACTOR` (Q=6) makes bandwidth proportional to centre frequency, so time resolution scales with 1/fc exactly as in a CWT/CQT. The only differences from a "wavelet filterbank" are the **kernel shape** (4th‑order Butterworth vs. Morlet/gammatone) and **real vs. analytic output**. So:

- **Denser features at the same latency?** Yes, but not because of wavelets — feature density (number of channels) and group latency are independent for *any* linear bank. Latency is set by per‑channel bandwidth (Gabor limit, σ_t·σ_f ≥ 1/4π), density is set by how many centre frequencies you place. Adding channels at the same Q costs 18 flops/sample/filter and zero extra latency, in your bank as much as in a wavelet one.
- **Same latency *and* same compute?** No. Nothing gives you more channels for free. Critically‑sampled DWT pyramids are cheaper per channel only because they decimate — and that trick works for dyadic (1 octave/band) splits, which are useless for semitone discrimination. Once you want 12+ voices/octave (rational‑dilation wavelets, CQT, gammatone) you are back to a bank of IIR/FIR bandpasses, i.e. what you have.
- **Where a wavelet‑type kernel genuinely wins:** analytic (complex) output and much better octave rejection. Those give *richer* features per channel at unchanged latency for ~1.8× compute — which is still negligible on your hardware.

## Measured comparison (same −3 dB bandwidth as your Q=6 design, fs=48 kHz)

I benchmarked your filter design against a 4th‑order complex gammatone (the standard "auditory wavelet", causal IIR) and a Morlet FIR (the canonical CWT kernel, linear phase), all tuned to identical −3 dB bandwidth (script: `/tmp/fb_compare.py`):

| fc | filter | gd@fc [ms] | rise 10/50/90 % [ms] | +1 semitone | +2 semitones | +1 oct | flops/sample |
|---|---|---|---|---|---|---|---|
| **82 Hz** | Butterworth N=2 (yours) | **32.9** | **11.5 / 33.3 / 61.4** | −1.1 dB | −7.1 dB | −38 dB | **18** |
| | Gammatone ord. 4 (complex) | 40.6 | 17.7 / 37.3 / 67.9 | −1.6 dB | −6.0 dB | −58 dB | 32 |
| | Morlet FIR ±3σ | 58.4 | 33.6 / 58.4 / 83.2 | −1.5 dB | −6.5 dB | −61 dB | 44 872 (5609 taps) |
| **330 Hz** | Butterworth (yours) | **8.2** | **2.9 / 8.3 / 15.3** | −1.1 | −7.1 | −38 | **18** |
| | Gammatone | 10.1 | 4.4 / 9.2 / 16.8 | −1.6 | −6.0 | −58 | 32 |
| | Morlet FIR | 14.5 | 8.4 / 14.5 / 20.7 | −1.5 | −6.5 | −61 | 11 160 |
| **659 Hz** | Butterworth (yours) | **4.1** | **1.5 / 4.2 / 7.7** | −1.1 | −7.1 | −38 | **18** |
| | Gammatone | 5.0 | 2.2 / 4.6 / 8.4 | −1.6 | −6.0 | −58 | 32 |
| | Morlet FIR | 7.3 | 4.2 / 7.3 / 10.4 | −1.5 | −6.5 | −61 | 5 592 |

Whole bank (148 filters): yours ≈ 128 MFLOP/s, complex gammatone ≈ 227 MFLOP/s. Both are noise next to the TFLite model on a 9950X3D.

Takeaways:

1. **Your Butterworth is the lowest‑latency option at a given bandwidth.** It is near minimum‑phase; 10 % rise at 82 Hz is 11.5 ms, matching the ~16 ms you measured in the README. The gammatone is ~20 % slower; the "optimal" Morlet is ~1.8× slower because a linear‑phase kernel must wait half its support. Wavelets do **not** beat you on group delay.
2. **Adjacent‑semitone rejection is identical (~−1 to −1.6 dB) for all three**, because it is dictated by the −3 dB width you chose (Q=6 → ±1.4 semitones wide). Your 148 channels are dense but heavily overlapping; semitone separation is currently done entirely by the NN, not the filters. No kernel change fixes that without narrowing bandwidth, and narrowing costs latency 1:1.
3. **Octave rejection is where you lose:** −38 dB vs −58/−61 dB. The 4th‑order Butterworth skirt is shallow, so harmonic channels leak into one another. That matters more for you than for a generic CQT because you explicitly build 4 harmonic channels per note.

## What would actually give the NN "denser" information at unchanged latency

In order of bang for the buck, given your code:

1. **Analytic output instead of `fabs(y1)`** (`filterbank.cpp:178`). Your rectified output has full‑wave ripple at 2·fc, which the model then has to average out. A complex resonator cascade (4 complex one‑poles ≈ the gammatone row above) gives a smooth envelope `|y|` *and* phase. From the phase derivative you get **instantaneous frequency per channel per sample** — sub‑bandwidth frequency resolution (the same principle as phase‑vocoder reassignment) with **no extra latency**. That is the one place where "denser features at the same latency" is actually achievable: two features per channel (magnitude + Δphase) instead of one noisy one. Cost: ~2× flops, i.e. ~230 MFLOP/s, and the model input becomes (148, 256, 2) → retrain.
2. **More centre frequencies at the same Q** (e.g. quarter‑tone grid, or 6 harmonics instead of 4). Zero latency impact; 18 flops/sample per added filter. Also a retrain.
3. **Multirate for the low octaves** (the only genuine wavelet "trick" that applies). The 82 Hz channel has a 13.6 Hz bandwidth; running the lowest two octaves at fs/8 would cut ~40 % of bank compute. Only worth it if you spend the savings on 1 or 2.
4. **Steeper skirts** (6th/8th‑order Butterworth, or the gammatone) for better harmonic isolation. Latency grows modestly (gammatone numbers above), rejection improves by ~20 dB.

What does **not** help: switching to a dyadic DWT (wrong resolution, shift‑variant due to decimation → jittery onsets), or a linear‑phase Morlet bank (1.8× latency, 300–2500× compute for nothing you can use in real time).

## Assumptions / limits

- Comparison assumes the model consumes per‑sample filter outputs as it does now (`(148, 256)` input, `model.py:31`); any feature change implies retraining.
- Latency figures are for isolated filters on a tone burst; the system‑level latency additionally includes the 256‑sample block wait in `noteinferencer.cpp:50` (5.3 ms) and smoothing.
- Flop counts are arithmetic only; your real SIMD throughput across 148 independent filters is good (contiguous state arrays), so even 2× is cheap.

If you want, I can prototype option 1 (complex 4‑pole resonator bank with magnitude + instantaneous‑frequency output) as a drop‑in alternative `FilterBank` behind a compile flag so you can A/B it in the training pipeline.


Here are the details of option 1, now with measured numbers (`/tmp/analytic_demo.py`) and a concrete mapping onto your code.

## 1. What `fabs(y1)` costs you today

`filterbank.cpp:178` emits `|y|` of a **real** bandpass output. On a steady 82 Hz tone the per‑sample feature looks like this:

```
rectified Butterworth |y| : min 0.000  max 1.000  mean 0.639   (full-wave ripple at 2·fc = 164 Hz)
analytic  |y|            : min 0.499  max 0.501  mean 0.500
```

So each channel delivers one scalar that swings 0→1 every half period. The `local_mean`/`local_contrast` layers and the first conv in `model.py` have to spend capacity integrating that ripple away, and at 82 Hz a 256‑sample window only contains ~1.4 periods, so the integration itself is noisy. Worse, the **phase** of the filter output — which encodes where inside the passband the energy actually sits — is thrown away entirely.

## 2. The analytic filter: same prototype, same latency

Instead of a real bandpass (poles at ±fc), build a **single‑sideband** filter: take the same 2nd‑order Butterworth *lowpass* prototype with cutoff bw/2 and rotate its poles and zeros by e^{j2πfc/fs}. The result has poles only near +fc, so its output is analytic (complex), and its magnitude response is just the LP response shifted to fc — identical −3 dB width and, since it is the same prototype, **identical group delay and rise time**:

```
                       rise 10/50/90 % @ 82 Hz
your real Butterworth:  11.5 / 33.3 / 61.4 ms
analytic Butterworth:   11.6 / 33.5 / 61.6 ms
```

Cost per filter per sample: the input is real, the two coefficients/states are complex → one 2‑pole complex IIR ≈ 22 flops vs your 18. For 148 filters that is ~160 MFLOP/s vs ~130 today. The previously quoted "2×" was for the 4‑pole gammatone; the SSB Butterworth is cheaper because it keeps order 2.

Design (replaces `design_butterworth_bandpass_N2`):

```cpp
// LP prototype: 2nd-order Butterworth, cutoff wc = bw/2, bilinear transform.
// Poles  p_k = (K + s_k)/(K - s_k), zeros at z = -1 (double).
// Rotate: p_k *= r, zeros -> -r   with r = exp(j*2*pi*fc/fs).
// Complex coefficients: a1 = -(p0+p1), a2 = p0*p1, b1 = 2r, b2 = r*r, b0 scaled for |H(fc)| = 1.
struct ComplexBiquad { std::complex<float> b0, b1, b2, a1, a2; };
```

## 3. The two features per channel

Given `y[n]` complex:

**Envelope**  `env[n] = |y[n]|` (or `|y|²` to avoid the sqrt — the model doesn't care, and your `noteinferencer` energy sum can use either consistently).

**Phase advance relative to fc** — the key new feature:

```
z[n]  = y[n] · conj(y[n-1]) · e^{-j2πfc/fs}
Δω[n] = arg z[n]                     →  f_inst = fc + Δω·fs/2π
```

`z` is a single complex multiply; `arg` is exact and needs no unwrapping because the deviation from fc is always ≪ π per sample. For the model you don't even need `atan2`: `Im(z) / (|y[n]|² + ε)` ≈ sin Δω, is division‑cheap, signed, and **self‑gating** — in silence it goes to 0 instead of producing random IF values:

```
silence/noise only:  raw IF std = 10.7 Hz   (garbage)
                     gated feature std = 0.0000
```

## 4. What the IF actually buys you (measured, 82 Hz channel, Q=6)

The channel's −3 dB edges are 75.2…88.8 Hz, i.e. the neighbouring semitones (77.8 and 87.3 Hz) are **inside** the passband at only −1 dB, which is why the filters alone can't separate semitones. The phase can:

```
tone 78.0 Hz -> |y| -6.5 dB,  IF = 78.00 ± 0.21 Hz,  within ±0.5 Hz after  7.8 ms
tone 82.0 Hz -> |y| -6.0 dB,  IF = 82.00 ± 0.20 Hz
tone 84.0 Hz -> |y| -6.1 dB,  IF = 84.00 ± 0.20 Hz,  within ±0.5 Hz after  8.5 ms
tone 87.3 Hz -> |y| -7.4 dB,  IF = 87.30 ± 0.24 Hz
tone 92.5 Hz -> |y| -14.2 dB, IF = 92.50 ± 0.51 Hz
```

A ±0.2 Hz per‑sample reading of a 5.3 Hz semitone gap is ~25σ separation, from a channel whose *magnitude* can't tell the two apart by more than 1 dB. Averaged over your 256‑sample block the jitter drops to ±0.03 Hz even with −30 dB white noise added. This is the "sub‑bandwidth resolution" of phase‑vocoder/reassignment methods, obtained with zero extra group delay: the IF locks within ~8 ms, while the envelope needs 33 ms to reach 50 %.

(The 73–78 ms settling for 87.3/92.5 Hz is a transient artefact of the hard tone onset — the channel initially rings at its own 82 Hz before the forced response dominates; with the gated `sin Δω` feature the NN sees that ring‑down at low amplitude and learns to ignore it.)

**Two partials in one channel** (82 + 87.3 Hz, equal level): the envelope beats 0.07↔0.93 at 5.3 Hz and the IF swings 50↔85 Hz at the beat rate. Both patterns are present *within one 256‑sample block only as a slow trend*, but across the 8 blocks in your `RING_BUFFER_SIZE` the NN would see a very distinctive AM+FM signature that the rectified magnitude smears out. That's the practical meaning of "denser features": per channel you go from one aliased scalar to a clean envelope plus a precise frequency pointer.

## 5. Implementation mapping

**C++ (`core/`)**

- `common.hpp`: add `#define NUM_FEATURES 2`; `AudioBuffer2D` grows to `num_filters × window_size × NUM_FEATURES` (or two planes — pick the layout that matches the TFLite input tensor).
- `filterbank.hpp`: coefficient arrays become complex (split `re/im` float arrays to keep the SIMD‑friendly SoA layout you have now); per filter one `std::complex<float> y_prev`.
- `filterbank.cpp::process`, inner loop sketch:

```cpp
// real input x, complex state (Transposed DF-II), coefficients complex
const cf y  = b0*x + s1;
s1 = b1*x - a1*y + s2;
s2 = b2*x - a2*y;
const float e2  = std::norm(y);                      // |y|^2
const cf    z   = y * std::conj(y_prev[f]) * rot_fc[f]; // rot_fc = exp(-j w_c)
out_env[n]  = std::sqrt(e2);                          // or e2
out_if[n]   = z.imag() / (e2 + eps);                  // gated sin(Δω)
y_prev[f] = y;
```

- `noteinferencer.cpp:85` currently sums the rectified magnitude for `note_energy`; switch it to the envelope plane — this will also make velocity less jittery since the 2·fc ripple is gone.
- `WITH_AUDIO_OUTPUT` debug path: output `y.real()`, which is the ordinary bandpass signal.

**Python training pipeline** — this must be mirrored exactly, otherwise train/inference mismatch:

- `python/neuralnetmodelling/fretboard.py:37-40` builds the same `signal.butter(N,[low,high])` + `np.abs(lfilter(...))`. Replace with the rotated LP prototype (`signal.butter(2, bw/2, 'low', output='zpk')`, multiply `z,p` by `exp(2jπfc/fs)`, `zpk2tf`) and emit the two feature planes. `fretboardnonredundant.py` likewise.
- `write_tfrecord_parallel.py:66` stores `int8(feature*127)`. The envelope stays in 0…1; `sin Δω` is in −1…1, so int8 is fine and the comment there about "negative values in the future" becomes true. Worth checking the quantisation step: ±0.5 Hz at 82 Hz is Δω ≈ 6.5e‑5 rad/sample → `sin Δω·127 ≈ 0.008` LSB, i.e. **int8 would destroy the IF feature**. Scale that plane by something like `fs/(2π·bw_channel)` (per‑channel normalisation to ±1 over the passband) before quantising, and apply the identical scale in C++.
- `common.py` `INPUT_SHAPE` and `model.py:333` input become `(148, 256, 2)`; the `local_mean`/`local_contrast` stage should only apply to the envelope plane.
- Retrain. The `freq_compress_conv` first layer will now see a well‑behaved input, so you may be able to shrink `BUFFER_SIZE`‑side pooling later.

## 6. Caveats

- The IF is a point estimate per channel: it is accurate when one partial dominates the passband, and swings when two are present. That swing is informative but you should not post‑process it as a pitch reading; let the model consume it.
- `ε` in the gate sets the noise floor; tie it to your onset energy threshold scale (gain‑dependent, see `noteinferencer.cpp:65`) rather than a hard constant.
- Group delay is unchanged, so the ~16 ms low‑E latency you measured stays; the gain is in robustness and semitone discrimination, not raw speed. If you want lower latency you still have to widen the low channels (and let the IF compensate for the lost magnitude selectivity — which is exactly the trade this feature enables).
- Per‑channel compute rises ~20 %, memory for the input tensor doubles.

If you want to go ahead, the natural first step is the Python side (`fretboardnonredundant.py` + a TFRecord of a small subset) to check training‑set separability before touching the C++ bank; I can set that up.