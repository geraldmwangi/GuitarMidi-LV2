#pragma once
#include <noteclassifier.hpp>
#include <iostream>
using namespace std;

class HarmonicGroup
{
    private:
    vector<shared_ptr<NoteClassifier> > m_noteClassifiers;
    
    bool m_oldMidiSentStatus;
    float* m_buffer;
    int m_bufferSize;
    public:

    float* audioBuffer;
    HarmonicGroup();
    ~HarmonicGroup();

    void addNoteClassifier(shared_ptr<NoteClassifier> notecl);

    void process(int nsamples);

    void sendMidi(int nsamples);
    
    bool m_wantToSendMidiStatus;





};
