#pragma once
#include <Graph.hpp>
#include <JuceHeader.h>

class MeanResponseGraph : public Graph
{
protected:
    virtual void computeGraph();
    juce::AudioSampleBuffer m_audioslice;

public:
    MeanResponseGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice);
    virtual ~MeanResponseGraph();
};


class AudioResponseGraph : public Graph
{
protected:
    virtual void computeGraph();
    juce::AudioSampleBuffer m_audioslice;

public:
    AudioResponseGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice);
    virtual ~AudioResponseGraph();
    virtual void drawGraph(juce::Graphics& g, juce::Rectangle<int> bounds );
};