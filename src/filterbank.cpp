// filterbank.cpp
#include "filterbank.hpp"
#include <complex>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using namespace GuitarMidi;
using cd = std::complex<double>;


// Design a 4th-order Butterworth bandpass filter using the bilinear transform method.
static void design_butterworth_bandpass_N2(
    double fc, double bw, double fs,
    float b0[2], float b1[2], float b2[2],
    float a1[2], float a2[2])
{

    const double f_lo = fc - bw * 0.5;
    const double f_hi = fc + bw * 0.5;


    const double w_lo = 2.0 * fs * std::tan(M_PI * f_lo / fs);
    const double w_hi = 2.0 * fs * std::tan(M_PI * f_hi / fs);
    const double w0   = std::sqrt(w_lo * w_hi);   // analog center (rad/s)
    const double BW   = w_hi - w_lo;              // analog bandwidth (rad/s)


    cd lp_poles[2];
    lp_poles[0] = cd(-std::sin(M_PI / 4.0),  std::cos(M_PI / 4.0));
    lp_poles[1] = std::conj(lp_poles[0]);


    cd bp_poles[4];
    for (int k = 0; k < 2; ++k) {
        const cd hp   = 0.5 * BW * lp_poles[k];
        const cd disc = std::sqrt(hp * hp - cd(w0 * w0, 0.0));
        bp_poles[2*k    ] = hp + disc;
        bp_poles[2*k + 1] = hp - disc;
    }

    const double K = 2.0 * fs;
    cd z_poles[4];
    for (int i = 0; i < 4; ++i)
        z_poles[i] = (K + bp_poles[i]) / (K - bp_poles[i]);

    cd z_zeros[4];
    z_zeros[0] = cd( 1.0, 0.0);
    z_zeros[1] = cd( 1.0, 0.0);
    z_zeros[2] = cd(-1.0, 0.0);
    z_zeros[3] = cd(-1.0, 0.0);


    const cd p_sec[2][2] = {
        { z_poles[0], z_poles[2] },   // section 0 (conjugate pair)
        { z_poles[1], z_poles[3] }    // section 1 (conjugate pair)
    };
    const cd z_sec[2][2] = {
        { cd( 1.0, 0.0), cd(-1.0, 0.0) },   // one +1, one -1
        { cd( 1.0, 0.0), cd(-1.0, 0.0) }
    };

    double sec_b0[2], sec_b1[2], sec_b2[2];
    double sec_a1[2], sec_a2[2];

    for (int s = 0; s < 2; ++s) {
        // Numerator from zeros z1, z2:
        //   (1 - z1 z^-1)(1 - z2 z^-1) = 1 - (z1+z2) z^-1 + z1 z2 z^-2
        const cd zsum = z_sec[s][0] + z_sec[s][1];
        const cd zprd = z_sec[s][0] * z_sec[s][1];
        sec_b0[s] = 1.0;
        sec_b1[s] = -zsum.real();
        sec_b2[s] =  zprd.real();

        // Denominator from poles:
        const cd psum = p_sec[s][0] + p_sec[s][1];   // real (conjugate pair)
        const cd pprd = p_sec[s][0] * p_sec[s][1];   // real
        sec_a1[s] = -psum.real();
        sec_a2[s] =  pprd.real();
    }


    const double wc = 2.0 * M_PI * fc / fs;
    const cd ejw   = std::exp(cd(0.0, -wc));        // z^-1 at center
    const cd ejw2  = ejw * ejw;                     // z^-2

    cd H = cd(1.0, 0.0);
    for (int s = 0; s < 2; ++s) {
        const cd num = sec_b0[s] + sec_b1[s] * ejw + sec_b2[s] * ejw2;
        const cd den = 1.0       + sec_a1[s] * ejw + sec_a2[s] * ejw2;
        H *= num / den;
    }
    const double scale = 1.0 / std::abs(H);

    sec_b0[0] *= scale;
    sec_b1[0] *= scale;
    sec_b2[0] *= scale;


    for (int s = 0; s < 2; ++s) {
        b0[s] = (float)sec_b0[s];
        b1[s] = (float)sec_b1[s];
        b2[s] = (float)sec_b2[s];
        a1[s] = (float)sec_a1[s];
        a2[s] = (float)sec_a2[s];
    }
}

// Design and process a 4th-order Butterworth bandpass filter using Transposed Direct Form II.

void FilterBank::setup(std::map<unsigned, FilterRepresentation> filterreps,
                       int samplerate)
{
    

    for (auto& kv : filterreps) {
        const int    fid = kv.second.filter_id;
        const double fc  = kv.second.center_freq;
        const double bw  = fc / Q_FACTOR;

        design_butterworth_bandpass_N2(
            fc, bw, (double)samplerate,
            m_b0[fid], m_b1[fid], m_b2[fid],
            m_a1[fid], m_a2[fid]);
    }

    reset();
}
void FilterBank::reset()
{
    std::memset(m_s1, 0, sizeof(m_s1));
    std::memset(m_s2, 0, sizeof(m_s2));
}


void FilterBank::process(int nsamples)
{
    for (int f = 0; f < NUM_FILTERS; ++f) {
        const float b0_0 = m_b0[f][0], b1_0 = m_b1[f][0], b2_0 = m_b2[f][0];
        const float a1_0 = m_a1[f][0], a2_0 = m_a2[f][0];
        const float b0_1 = m_b0[f][1], b1_1 = m_b1[f][1], b2_1 = m_b2[f][1];
        const float a1_1 = m_a1[f][1], a2_1 = m_a2[f][1];

        float s1_0 = m_s1[f][0], s2_0 = m_s2[f][0];
        float s1_1 = m_s1[f][1], s2_1 = m_s2[f][1];

        float* __restrict out =
            m_filterbankbuffer.audio_buffer_2D + f * m_filterbankbuffer.window_size;

        for (int n = 0; n < nsamples; ++n) {
            const float x = m_input[n];

            // Section 0 (Transposed Direct Form II)
            const float y0 = b0_0 * x + s1_0;
            s1_0 = b1_0 * x - a1_0 * y0 + s2_0;
            s2_0 = b2_0 * x - a2_0 * y0;

            // Section 1, input = y0
            const float y1 = b0_1 * y0 + s1_1;
            s1_1 = b1_1 * y0 - a1_1 * y1 + s2_1;
            s2_1 = b2_1 * y0 - a2_1 * y1;

            out[n] = std::fabs(y1);
        }

        m_s1[f][0] = s1_0;  m_s2[f][0] = s2_0;
        m_s1[f][1] = s1_1;  m_s2[f][1] = s2_1;
    }
}

