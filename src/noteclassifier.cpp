#include <noteclassifier.hpp>

NoteClassifier::NoteClassifier(float samplerate, float center, float bandwidth, float passbandatten)
{
    m_centerfreq=center;
    m_passbandatten=passbandatten;
    m_bandwidth=bandwidth;
    m_samplerate=samplerate;
}

void NoteClassifier::initialize()
{
    for(int i=0;i<FILTERORDER;i++)
    {
        m_filter[i].reset();
        m_filter[i].setup(1, m_samplerate, m_centerfreq, m_bandwidth, m_passbandatten, 60.0);
    }
}

void NoteClassifier::finalize()
{
    for(int i=0;i<FILTERORDER;i++)
        m_filter[i].reset();
}

void NoteClassifier::process(int nsamples)
{
    memcpy(output,input,nsamples*sizeof(float));
    for(int i=0;i<FILTERORDER;i++)
        m_filter[i].process(nsamples,&output);

    float meanenv=0;
    int count=0;

    for(int s=1;s<(nsamples-1);s++)
    {
        if(output[s]>output[s-1]&&output[s]>output[s+1]&&output[s]>0)
            {
                meanenv+=output[s];
                count++;
            }
    }
    if(count)
        meanenv/=count;

    if(meanenv>0.1)
        printf("got signal: %f\n",meanenv);

}