#pragma once
#include <noteclassifier.hpp>

using namespace std;

class HarmonicGroup
{
    private:
    float m_fundamentalFreq;
    vector<shared_ptr<NoteClassifier> > m_noteClassifiers;

    public:
    HarmonicGroup(float fundamentalFreq);

    void addNoteClassifier(shared_ptr<NoteClassifier> notecl);

    void process(int nsamples);






};
