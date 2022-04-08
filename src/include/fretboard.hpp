#pragma once
#include <noteclassifier.hpp>
#include <memory>
#include <vector>
typedef enum
{
    FRETBOARD_INPUT = 0,
    FRETBOARD_OUTPUT = 1,
    FRETBOARD_MIDIOUTPUT=2
} PortIndex;
using namespace std;
class FretBoard
{
private:
    vector<shared_ptr<NoteClassifier>> m_noteClassifiers;

public:
    FretBoard(LV2_URID_Map *map, float samplerate);

    void setAudioInput(const float *input);
    void setAudioOutput(float *output);
    void setMidiOutput(LV2_Atom_Sequence *output);

    void initialize();

    void finalize();

    void process(int nsamples);
};