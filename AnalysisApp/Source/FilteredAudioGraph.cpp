#include <FilteredAudioGraph.hpp>

FilteredAudioGraph::FilteredAudioGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice) : Graph(notecl)
{
    m_audioslice=audioslice;
}

FilteredAudioGraph::~FilteredAudioGraph()
{
}

void FilteredAudioGraph::computeGraph()
{
    float minf = 0.0;
    float maxf = 500.0;
    float maxVal=0.1;
    m_initialized = false;
    if(m_audioslice.getNumSamples())
    {
        float meanenv=m_notecl->filterAndComputeMeanEnv(*m_audioslice.getArrayOfWritePointers(),m_audioslice.getNumSamples());
        m_path.addRectangle(m_notecl->getCenterFrequency(),maxVal-meanenv,10,meanenv);
    }
    addFunctionPoint(minf,maxVal);
    addFunctionPoint(maxf,maxVal);

    m_initialized=false;
    addFunctionPoint(minf,0.0);
    addFunctionPoint(maxf,0.0);


}