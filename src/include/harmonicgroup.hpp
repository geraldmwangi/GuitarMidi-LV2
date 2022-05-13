#pragma once
#include <noteclassifier.hpp>

using namespace std;

class HarmonicGroup
{
    private:
    vector<shared_ptr<NoteClassifier> > m_noteClassifiers;

    public:
    HarmonicGroup();

    void addNoteClassifier(shared_ptr<NoteClassifier> notecl);

    void process(int nsamples);






};
