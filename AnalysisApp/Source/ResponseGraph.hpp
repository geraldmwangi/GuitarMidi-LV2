#pragma once
#include <Graph.hpp>
#include <JuceHeader.h>

class ResponseGraph : public Graph
{
protected:
    virtual void computeGraph();
    juce::AudioSampleBuffer m_audioslice;

public:
    ResponseGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice);
    virtual ~ResponseGraph();
};