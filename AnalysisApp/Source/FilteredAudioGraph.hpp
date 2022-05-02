#pragma once
#include <Graph.hpp>
#include <JuceHeader.h>

class FilteredAudioGraph : public Graph
{
protected:
    virtual void computeGraph();
    juce::AudioSampleBuffer m_audioslice;

public:
    FilteredAudioGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice);
    virtual ~FilteredAudioGraph();
};