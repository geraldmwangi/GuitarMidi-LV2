"""Compare GuitarMidi's 4th-order Butterworth BP bank (Q=6) against wavelet-style
filters with the SAME -3 dB bandwidth: Morlet/Gabor FIR (linear phase) and a
4th-order complex gammatone IIR (causal, near minimum phase).
Metrics: group delay @fc, envelope rise time to 10/50/90% on a tone burst,
rejection at +-1 semitone, +-2 semitones, +-1 octave, flops/sample.
"""
import numpy as np
from scipy import signal

FS = 48000
Q = 6
SEMI = 2 ** (1 / 12)


def butter_sos(fc):
    bw = fc / Q
    return signal.butter(2, [fc - bw / 2, fc + bw / 2], "bandpass", fs=FS, output="sos")


def dtft(h_imp, f):
    n = np.arange(len(h_imp))
    f = np.atleast_1d(f)
    out = np.empty(len(f), dtype=complex)
    for i in range(0, len(f), 64):  # chunked outer product
        E = np.exp(-2j * np.pi * np.outer(f[i:i + 64], n) / FS)
        out[i:i + 64] = E @ h_imp
    return out


def resp_db(h_imp, f):
    """Magnitude response (dB) of an impulse response at frequencies f, by DTFT."""
    return 20 * np.log10(np.abs(dtft(h_imp, f)) + 1e-30)


def bw3_from_impulse(h_imp, fc):
    f = np.linspace(fc * 0.7, fc * 1.3, 1201)
    H = resp_db(h_imp, f)
    H -= H.max()
    inside = f[H >= -3.0103]
    return inside[-1] - inside[0]


def group_delay_ms(h_imp, fc):
    df = 0.5
    f = np.array([fc - df, fc + df])
    n = np.arange(len(h_imp))
    H = np.array([np.sum(h_imp * np.exp(-2j * np.pi * fi / FS * n)) for fi in f])
    dphi = np.angle(H[1] / H[0])  # unwrapped small difference
    gd = -dphi / (2 * np.pi * 2 * df)
    return gd * 1000


def rise_times_ms(env, steady):
    out = []
    for frac in (0.1, 0.5, 0.9):
        idx = np.argmax(env >= frac * steady)
        out.append(idx / FS * 1000)
    return out


def analyze_butter(fc):
    sos = butter_sos(fc)
    imp = signal.sosfilt(sos, np.r_[1.0, np.zeros(int(FS * 1.0))])
    t = np.arange(int(FS * 0.6)) / FS
    pad = int(FS * 0.1)  # keep the Hilbert edge artefact away from the onset
    x = np.r_[np.zeros(pad), np.sin(2 * np.pi * fc * t)]
    y = signal.sosfilt(sos, x)
    env = np.abs(signal.hilbert(y))[pad:]
    steady = env[int(FS * 0.4):].mean()
    return dict(name="Butterworth N=2 (yours)", bw3=bw3_from_impulse(imp, fc),
                gd=group_delay_ms(imp, fc), rise=rise_times_ms(env, steady), imp=imp,
                flops=2 * 9, taps="2 biquads", note="real output -> |y| ripple @2fc")


def gammatone_complex(fc, b, n_samples):
    """4 cascaded complex one-pole resonators = order-4 gammatone, analytic output."""
    p = np.exp(-2 * np.pi * b / FS) * np.exp(2j * np.pi * fc / FS)
    x = np.r_[1.0, np.zeros(n_samples - 1)].astype(complex)
    y = x
    for _ in range(4):
        y = signal.lfilter([1 - abs(p)], [1, -p], y)
    return y


def analyze_gammatone(fc, target_bw3):
    # closed form: n cascaded identical 1-pole resonators (bandwidth b each) have
    # |H|^2 = (1 + (f/b)^2)^-n  ->  -3 dB full width = 2 b sqrt(2^(1/n) - 1)
    b = target_bw3 / (2 * np.sqrt(2 ** (1 / 4) - 1))
    imp = gammatone_complex(fc, b, int(FS * 1.0))
    imp = imp / np.abs(np.sum(imp * np.exp(-2j * np.pi * fc / FS * np.arange(len(imp)))))
    t = np.arange(int(FS * 0.6)) / FS
    y = signal.lfilter(imp[: int(FS * 0.5)], [1], np.sin(2 * np.pi * fc * t))
    env = np.abs(y)  # analytic -> envelope directly, no Hilbert / rectification
    steady = env[int(FS * 0.4):].mean()
    return dict(name="Gammatone order 4 (complex IIR)", bw3=bw3_from_impulse(imp, fc),
                gd=group_delay_ms(imp, fc), rise=rise_times_ms(env, steady), imp=imp,
                flops=4 * 8, taps="4 complex 1-pole", note="analytic -> smooth envelope")


def analyze_morlet(fc, target_bw3, nsig=3.0):
    # Gaussian magnitude: -3dB full width = 2*sqrt(ln2)*sigma_f
    sigma_f = target_bw3 / (2 * np.sqrt(np.log(2)))
    sigma_t = 1 / (2 * np.pi * sigma_f)
    half = int(np.ceil(nsig * sigma_t * FS))
    n = np.arange(-half, half + 1)
    w = np.exp(-0.5 * (n / FS / sigma_t) ** 2)
    imp = w * np.exp(2j * np.pi * fc * n / FS)
    imp = imp / np.abs(np.sum(imp * np.exp(-2j * np.pi * fc / FS * np.arange(len(imp)))))
    t = np.arange(int(FS * 0.6)) / FS
    y = signal.lfilter(imp, [1], np.sin(2 * np.pi * fc * t))
    env = np.abs(y)
    steady = env[int(FS * 0.4):].mean()
    return dict(name=f"Morlet FIR (+-{nsig:.0f} sigma, linear phase)", bw3=bw3_from_impulse(imp, fc),
                gd=group_delay_ms(imp, fc), rise=rise_times_ms(env, steady), imp=imp,
                flops=len(imp) * 8, taps=f"{len(imp)} complex taps", note="delay = half support")


def rejection(imp, fc):
    H0 = resp_db(imp, fc)[0]
    f = np.array([fc * SEMI, fc * SEMI ** 2, fc * 2, fc / 2])
    return resp_db(imp, f) - H0


if __name__ == "__main__":
    for fc in (82.0, 165.0, 330.0, 659.0):
        print(f"\n=== fc = {fc:.0f} Hz   (period {1000/fc:.2f} ms, Q={Q} -> design bw {fc/Q:.1f} Hz) ===")
        b = analyze_butter(fc)
        results = [b, analyze_gammatone(fc, b["bw3"]), analyze_morlet(fc, b["bw3"])]
        print(f"{'filter':38s} {'bw3[Hz]':>8s} {'gd@fc[ms]':>10s} {'rise10/50/90[ms]':>20s} "
              f"{'+1st':>6s} {'+2st':>6s} {'+1oct':>6s} {'-1oct':>6s} {'flops/smp':>10s}  cost")
        for r in results:
            rej = rejection(r["imp"], fc)
            print(f"{r['name']:38s} {r['bw3']:8.2f} {r['gd']:10.2f} "
                  f"{r['rise'][0]:6.1f}/{r['rise'][1]:5.1f}/{r['rise'][2]:5.1f}   "
                  f"{rej[0]:6.1f} {rej[1]:6.1f} {rej[2]:6.1f} {rej[3]:6.1f} {r['flops']:10d}  {r['taps']}")
    print("\nWhole-bank cost (148 filters) per audio sample at 48 kHz:")
    print(f"  Butterworth bank (current):   {148*18:>8d} flops/sample = {148*18*FS/1e6:6.1f} MFLOP/s")
    print(f"  Complex gammatone bank:       {148*32:>8d} flops/sample = {148*32*FS/1e6:6.1f} MFLOP/s")
