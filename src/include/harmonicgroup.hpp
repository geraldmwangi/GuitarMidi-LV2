#pragma once
#include <noteclassifier.hpp>
#include <iostream>
using namespace std;

class HarmonicGroup
{
    private:
    vector<shared_ptr<NoteClassifier> > m_noteClassifiers;
    bool m_oldState;

    float m_onsetThresh;
    float m_onsetSilence;
    float m_onsetCompression;
    int m_onsetBuffersize;
    string m_onsetMethod;
    aubio_onset_t* m_onsetDetector;
    int m_samplerate;

    float getMeanEnv(int nsamples,bool *period_over);
    float m_meanEnv;
    int m_meanEnvCounter;

    public:
    float* m_buffer;
    int m_bufferSize;

    float* audioBuffer;
    HarmonicGroup();
    ~HarmonicGroup();
    void setOnsetParameter(string method,float threshold=0.4,float silence=-40,float comp=0.0,int onsetbuffersize=512,bool adap_whitening=false);

    void addNoteClassifier(shared_ptr<NoteClassifier> notecl);

    void resetFilters();

    void filterAndSumBuffers(const float *input,int nsamples,bool* onsetdetected=nullptr);

    void process(int nsamples);






};
