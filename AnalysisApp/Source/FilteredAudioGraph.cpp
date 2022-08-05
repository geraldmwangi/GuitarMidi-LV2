#include <FilteredAudioGraph.hpp>

MeanResponseGraph::MeanResponseGraph(shared_ptr<NoteClassifier> notecl, juce::AudioSampleBuffer audioslice) : Graph(notecl)
{
    m_audioslice.makeCopyOf(audioslice);
}

MeanResponseGraph::~MeanResponseGraph()
{
}

void MeanResponseGraph::computeGraph()
{
    float minf = 0.0;
    float maxf = 4000.0;
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
    m_audioslice.makeCopyOf(audioslice);
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
    // res.makeCopyOf(m_audioslice);
    res.setSize(2,m_audioslice.getNumSamples());
    res.copyFrom(0,0,*m_audioslice.getArrayOfReadPointers(),m_audioslice.getNumSamples());
    res.copyFrom(1,0,*m_audioslice.getArrayOfReadPointers(),m_audioslice.getNumSamples());
    int chunk=256;
    int onscount=0;
    m_notecl->resetFilterAndOnsetDetector();
    int startpos=round(((float)(g.getClipBounds().getX()-bounds.getX()))/bounds.getWidth()*m_audioslice.getNumSamples());
    int endpos=round(((float)(g.getClipBounds().getX()+g.getClipBounds().getWidth()+-bounds.getX()))/bounds.getWidth()*m_audioslice.getNumSamples());
    for(int pos=startpos;pos<min(endpos,m_audioslice.getNumSamples()-chunk);pos+=chunk)
    {
       bool onset=0;
       m_notecl->filterAndComputeMeanEnv(res.getArrayOfWritePointers()[0]+pos,chunk,&onset);
       memcpy(res.getArrayOfWritePointers()[0]+pos,m_notecl->m_buffer,sizeof(float)*chunk);

       if(onset)
        {
            float linepos=(((float)pos)/m_audioslice.getNumSamples()*bounds.getWidth());
            g.drawLine(linepos,0,linepos,bounds.getHeight());
            onscount++;
        }

    }
    juce::AudioFormatManager manager;
    juce::AudioThumbnailCache cache(1);
    float factor=((float)g.getClipBounds().getWidth())/bounds.getWidth();
    int thumbres=256*factor;
    thumbres=(thumbres>1)?thumbres:1;
    juce::AudioThumbnail thumbnail(thumbres,manager,cache);
    thumbnail.reset(2,48000,res.getNumSamples());
    thumbnail.addBlock(0,res,0,res.getNumSamples());
    // thumbnail.addBlock(1,m_audioslice,0,m_audioslice.getNumSamples());
    float start=((float)startpos)/m_audioslice.getNumSamples()*thumbnail.getTotalLength();//((float)(g.getClipBounds().getX()-bounds.getX()))/bounds.getWidth()*thumbnail.getTotalLength();
    float end=((float)endpos)/m_audioslice.getNumSamples()*thumbnail.getTotalLength();//((float)(g.getClipBounds().getX()+g.getClipBounds().getWidth()+-bounds.getX()))/bounds.getWidth()*thumbnail.getTotalLength();
    thumbnail.drawChannels(g,g.getClipBounds(),start,end,1.0);
    cout<<"Found "<<onscount<<" onsets"<<endl;
    
}


















HarmonicGroupResponseGraph::HarmonicGroupResponseGraph(shared_ptr<HarmonicGroup> group, juce::AudioSampleBuffer audioslice)
{
    m_audioslice.makeCopyOf(audioslice);
    m_group=group;
}

HarmonicGroupResponseGraph::~HarmonicGroupResponseGraph()
{
}

void HarmonicGroupResponseGraph::computeGraph()
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

void HarmonicGroupResponseGraph::drawGraph(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    AudioSampleBuffer res;
    // res.makeCopyOf(m_audioslice);
    res.setSize(2,m_audioslice.getNumSamples());
    res.copyFrom(0,0,*m_audioslice.getArrayOfReadPointers(),m_audioslice.getNumSamples());
    res.copyFrom(1,0,*m_audioslice.getArrayOfReadPointers(),m_audioslice.getNumSamples());
    int chunk=256;
    int onscount=0;
    m_group->resetFilters();
    int startpos=round(((float)(g.getClipBounds().getX()-bounds.getX()))/bounds.getWidth()*m_audioslice.getNumSamples());
    int endpos=round(((float)(g.getClipBounds().getX()+g.getClipBounds().getWidth()+-bounds.getX()))/bounds.getWidth()*m_audioslice.getNumSamples());
    for(int pos=startpos;pos<min(endpos,m_audioslice.getNumSamples()-chunk);pos+=chunk)
    {
       bool onset=0;
       m_group->filterAndSumBuffers(res.getArrayOfWritePointers()[0]+pos,chunk,&onset);
       memcpy(res.getArrayOfWritePointers()[0]+pos,m_group->m_buffer,chunk*sizeof(float));
 

       if(onset)
        {
            float linepos=(((float)pos)/m_audioslice.getNumSamples()*bounds.getWidth());
            g.drawLine(linepos,0,linepos,bounds.getHeight());
            onscount++;
        }

    }
    juce::AudioFormatManager manager;
    juce::AudioThumbnailCache cache(1);
    float factor=((float)g.getClipBounds().getWidth())/bounds.getWidth();
    int thumbres=256*factor;
    thumbres=(thumbres>1)?thumbres:1;
    juce::AudioThumbnail thumbnail(thumbres,manager,cache);
    thumbnail.reset(2,48000,res.getNumSamples());
    thumbnail.addBlock(0,res,0,res.getNumSamples());
    // thumbnail.addBlock(1,m_audioslice,0,m_audioslice.getNumSamples());
    float start=((float)startpos)/m_audioslice.getNumSamples()*thumbnail.getTotalLength();//((float)(g.getClipBounds().getX()-bounds.getX()))/bounds.getWidth()*thumbnail.getTotalLength();
    float end=((float)endpos)/m_audioslice.getNumSamples()*thumbnail.getTotalLength();//((float)(g.getClipBounds().getX()+g.getClipBounds().getWidth()+-bounds.getX()))/bounds.getWidth()*thumbnail.getTotalLength();
    thumbnail.drawChannels(g,g.getClipBounds(),start,end,1.0);
    cout<<"Found "<<onscount<<" onsets"<<endl;
    
}