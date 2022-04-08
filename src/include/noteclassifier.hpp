#pragma once
#include <DspFilters/Dsp.h>
#include <aubio/aubio.h>
#include <aubio/pitch/pitch.h>
#include <midioutput.hpp>

#define FILTERORDER 4 // the real order is 2*MAXORDER
#define MAXORDER 4 // the real order is 2*MAXORDER
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

    aubio_pitch_t* mPitchDetector;
    fvec_t* m_pitchfreq;
    float* m_pitchbuffer;
    int m_pitchBufferCounter;
    int mInBufSize;
    int mBufferSize;

    bool m_noteOnOffState;
    bool m_oldNoteOnOffState;

    Dsp::SimpleFilter <Dsp::Elliptic::BandPass<MAXORDER>, 1> m_filter[FILTERORDER];

    MidiOutput m_midiOutput;

public:
    NoteClassifier(LV2_URID_Map *map,float samplerate, float center = 110.0, float bandwidth = 20, float passbandatten = 5);

    void initialize();

    void finalize();

    void process(int nsamples);

    const float *input;
    float *output;

    void setMidiOutput(LV2_Atom_Sequence* output);
};
