#pragma once
#include <noteclassifier.hpp>
#include <iostream>
using namespace std;

class HarmonicGroup
{
    private:
    vector<shared_ptr<NoteClassifier> > m_noteClassifiers;
    bool m_oldState;
    float* m_buffer;
    int m_bufferSize;
    public:

    float* audioBuffer;
    HarmonicGroup();
    ~HarmonicGroup();

    void addNoteClassifier(shared_ptr<NoteClassifier> notecl);

    void process(int nsamples);

    bool getState(){
        return m_oldState;
    }






};
