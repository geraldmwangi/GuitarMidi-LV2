#pragma once
#include <DspFilters/Dsp.h>

typedef enum
{
    NOTECL_INPUT = 0,
    NOTECL_OUTPUT = 1
} PortIndex;
#define FILTERORDER 4 // the real order is 2*MAXORDER
#define MAXORDER 20 // the real order is 2*MAXORDER
/**
 * @brief Implementing an elliptic band pass filter from the DSPFilters repo
 *
 */
class NoteClassifier
{
private:
    float m_centerfreq;
    float m_bandwidth;
    float m_passbandatten;
    float m_samplerate;

    Dsp::SimpleFilter <Dsp::Elliptic::BandPass<MAXORDER>, 1> m_filter[FILTERORDER];

public:
    NoteClassifier(float samplerate, float center = 110.0, float bandwidth = 40, float passbandatten = 5);

    void initialize();

    void finalize();

    void process(int nsamples);

    const float *input;
    float *output;
};
