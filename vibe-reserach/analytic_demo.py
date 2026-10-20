"""Demo of analytic (complex) resonator output vs rectified real Butterworth.
Channel: fc = 82 Hz, Q = 6 (bw = 13.7 Hz). Same -3 dB width for both.
"""
import numpy as np
from scipy import signal

FS = 48000
Q = 6


def butter_sos(fc):
    bw = fc / Q
    return signal.butter(2, [fc - bw / 2, fc + bw / 2], "bandpass", fs=FS, output="sos")


def complex_butter_coeffs(fc):
    """Single-sideband Butterworth: 2nd-order LP prototype (cutoff bw/2) with poles
    rotated to +fc. Output is analytic; magnitude above DC == |H_LP(f - fc)|."""
    bw = fc / Q
    z, p, k = signal.butter(2, bw / 2, "lowpass", fs=FS, output="zpk")
    rot = np.exp(2j * np.pi * fc / FS)
    p = p * rot
    z = z * rot
    b, a = signal.zpk2tf(z, p, k)
    # normalise to unity gain at fc
    w, h = signal.freqz(b, a, worN=[2 * np.pi * fc / FS])
    return b / np.abs(h[0]), a


def features(y, fc):
    """Envelope and instantaneous frequency deviation from the analytic signal."""
    env = np.abs(y)
    rot = np.exp(-2j * np.pi * fc / FS)
    z = y[1:] * np.conj(y[:-1]) * rot            # phase advance relative to fc
    dw = np.angle(z)                             # rad/sample, exact
    f_inst = fc + dw * FS / (2 * np.pi)
    eps = 1e-4 ** 2
    sin_dw_gated = z.imag / (env[1:] ** 2 + eps)  # cheap, division-free-ish, noise-gated
    return env, f_inst, sin_dw_gated


if __name__ == "__main__":
    fc = 82.0
    b, a = complex_butter_coeffs(fc)
    sos = butter_sos(fc)
    t = np.arange(int(FS * 0.5)) / FS

    print("=== 1. Ripple of the per-sample feature on a steady 82 Hz tone ===")
    x = np.sin(2 * np.pi * fc * t)
    y_r = np.abs(signal.sosfilt(sos, x))[int(FS * 0.3):]
    y_c = np.abs(signal.lfilter(b, a, x))[int(FS * 0.3):]
    print(f"  rectified Butterworth |y| : min {y_r.min():.3f} max {y_r.max():.3f} mean {y_r.mean():.3f}  (ripple at 2fc)")
    print(f"  analytic  |y|            : min {y_c.min():.3f} max {y_c.max():.3f} mean {y_c.mean():.3f}")

    print("\n=== 2. Latency identical? step response of envelope (10/50/90 %) ===")
    for name, env in (("rectified+hilbert", np.abs(signal.hilbert(signal.sosfilt(sos, x)))),
                      ("analytic", np.abs(signal.lfilter(b, a, x)))):
        steady = env[int(FS * 0.3):].mean()
        r = [np.argmax(env >= f * steady) / FS * 1000 for f in (0.1, 0.5, 0.9)]
        print(f"  {name:18s}: {r[0]:5.1f} / {r[1]:5.1f} / {r[2]:5.1f} ms")

    print("\n=== 3. Instantaneous frequency readout for off-centre tones through the 82 Hz channel ===")
    print("  (channel -3 dB edges are 75.2 .. 88.8 Hz; neighbours at 77.8 and 87.3 Hz)")
    for f_true in (78.0, 82.0, 84.0, 87.3, 92.5):
        x = np.sin(2 * np.pi * f_true * t)
        y = signal.lfilter(b, a, x)
        env, f_inst, _ = features(y, fc)
        seg = f_inst[int(FS * 0.3):]
        mag_db = 20 * np.log10(env[int(FS * 0.3):].mean())
        t50 = np.argmax(np.abs(f_inst - f_true) < 0.5) / FS * 1000
        print(f"  tone {f_true:6.1f} Hz -> |y| {mag_db:6.1f} dB,  IF = {seg.mean():6.2f} +- {seg.std():.3f} Hz,"
              f"  settles to +-0.5 Hz after {t50:5.1f} ms")

    print("\n=== 4. Same, with white noise at -30 dB re tone (per-sample IF noise and the gated feature) ===")
    rng = np.random.default_rng(0)
    for f_true in (82.0, 87.3):
        x = np.sin(2 * np.pi * f_true * t) + 10 ** (-30 / 20) * rng.standard_normal(len(t))
        y = signal.lfilter(b, a, x)
        env, f_inst, g = features(y, fc)
        seg = f_inst[int(FS * 0.3):]
        print(f"  tone {f_true:6.1f} Hz -> IF = {seg.mean():6.2f} +- {seg.std():.3f} Hz per sample; "
              f"averaged over 256 samples: +- {seg[:len(seg)//256*256].reshape(-1,256).mean(1).std():.3f} Hz")

    print("\n=== 5. Two partials inside one channel: 82 Hz + 87.3 Hz (minor 2nd, equal level) ===")
    x = np.sin(2 * np.pi * 82.0 * t) + np.sin(2 * np.pi * 87.3 * t)
    y = signal.lfilter(b, a, x)
    env, f_inst, _ = features(y, fc)
    seg = slice(int(FS * 0.3), None)
    print(f"  envelope beats: min {env[seg].min():.2f} max {env[seg].max():.2f} at {87.3-82:.1f} Hz")
    print(f"  IF swings: min {f_inst[seg].min():7.1f} max {f_inst[seg].max():7.1f} Hz (mean {f_inst[seg].mean():.1f})")
    print("  -> envelope AM + IF excursions at the beat rate are the two-note signature for the NN")

    print("\n=== 6. Silence / noise only: gated feature stays near zero, raw IF is garbage ===")
    x = 1e-3 * rng.standard_normal(len(t))
    y = signal.lfilter(b, a, x)
    env, f_inst, g = features(y, fc)
    print(f"  raw IF std: {f_inst[seg].std():8.2f} Hz   gated sin(dw)/(|y|^2+eps) std: {g[seg].std():.4f}")
