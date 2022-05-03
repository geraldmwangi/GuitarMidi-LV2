#include <FilteredAudioGraph.hpp>

MeanResponseGraph::MeanResponseGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice) : Graph(notecl)
{
    m_audioslice=audioslice;
}

MeanResponseGraph::~MeanResponseGraph()
{
}

void MeanResponseGraph::computeGraph()
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


AudioResponseGraph::AudioResponseGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice) : Graph(notecl)
{
    m_audioslice=audioslice;
}

AudioResponseGraph::~AudioResponseGraph()
{
}

void AudioResponseGraph::computeGraph()
{
    // float minf = 0.0;
    // float maxf = 500.0;
    // float maxVal=0.1;
    // m_initialized = false;
    // if(m_audioslice.getNumSamples())
    // {
    //     float meanenv=m_notecl->filterAndComputeMeanEnv(*m_audioslice.getArrayOfWritePointers(),m_audioslice.getNumSamples());
    //     m_path.addRectangle(m_notecl->getCenterFrequency(),maxVal-meanenv,10,meanenv);
    // }
    
    // addFunctionPoint(minf,maxVal);
    // addFunctionPoint(maxf,maxVal);

    // m_initialized=false;
    // addFunctionPoint(minf,0.0);
    // addFunctionPoint(maxf,0.0);


}

void AudioResponseGraph::drawGraph(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    AudioSampleBuffer res;
    res.setSize(2,m_audioslice.getNumSamples());
    int chunk=256;
    for(int pos=0;pos<(m_audioslice.getNumSamples()-chunk);pos+=chunk)
    {
       res.copyFrom(0,pos,*m_audioslice.getArrayOfReadPointers()+pos,chunk);
       //res.copyFrom(1,pos,*m_audioslice.getArrayOfReadPointers()+pos,chunk);
       m_notecl->filterAndComputeMeanEnv(*res.getArrayOfWritePointers()+pos,chunk);
       res.copyFrom(1,pos,*m_audioslice.getArrayOfReadPointers()+pos,chunk);

    }
    juce::AudioFormatManager manager;
    juce::AudioThumbnailCache cache(1);
    juce::AudioThumbnail thumbnail(512,manager,cache);
    thumbnail.reset(2,48000,res.getNumSamples());
    thumbnail.addBlock(0,res,0,res.getNumSamples());
    thumbnail.drawChannels(g,bounds,0,thumbnail.getTotalLength(),1.0);
    
}