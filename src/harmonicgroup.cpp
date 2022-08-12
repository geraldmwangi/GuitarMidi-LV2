#include <harmonicgroup.hpp>

HarmonicGroup::HarmonicGroup()
{
    m_oldState = false;
    m_bufferSize=256;
    m_samplerate = 48000;
    m_onsetDetector = nullptr;
    setOnsetParameter("energy");
    m_buffer=new float[m_bufferSize];
    audioBuffer=nullptr;
}

HarmonicGroup::~HarmonicGroup()
{
    if(m_buffer)
        delete [] m_buffer;
    if (m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
}

void HarmonicGroup::addNoteClassifier(shared_ptr<NoteClassifier> notecl)
{
    if (m_noteClassifiers.size() == 0)
        m_noteClassifiers.push_back(notecl); // Add fundamental
    else
    {
        for (int n = 2; n <= 12; n++)
        {
            float min = m_noteClassifiers[0]->getCenterFrequency() * n - 0.5;
            float max = m_noteClassifiers[0]->getCenterFrequency() * n + 0.5;
            float f = notecl->getCenterFrequency();
            if (f >= min && f <= max)
                m_noteClassifiers.push_back(notecl);
        }
    }
}
void HarmonicGroup::resetFilters()
{
    for (auto notecl : m_noteClassifiers)
        notecl->resetFilterAndOnsetDetector();

    if (m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
    m_onsetDetector = new_aubio_onset(m_onsetMethod.c_str(), m_onsetBuffersize, m_onsetBuffersize / 2, m_samplerate);
    aubio_onset_set_threshold(m_onsetDetector, m_onsetThresh);
    // aubio_onset_set_awhitening(m_onsetDetector,adap_whitening);
    aubio_onset_set_silence(m_onsetDetector, m_onsetSilence);
    aubio_onset_set_compression(m_onsetDetector, m_onsetCompression);
}
void HarmonicGroup::filterAndSumBuffers(const float *input,int nsamples,bool* onsetdetected)
{
    memset(m_buffer, 0, nsamples * sizeof(float));
    for (auto notecl : m_noteClassifiers)
    {
        notecl->filterAndComputeMeanEnv(input, nsamples);

        for (int s = 0; s < nsamples; s++)
            m_buffer[s] += notecl->m_buffer[s];
    }
    if (m_onsetDetector)
    {
        fvec_t *ons = new_fvec(1);
        fvec_t onsinput;
        onsinput.data = (smpl_t *)m_buffer;
        onsinput.length = nsamples;

        aubio_onset_do(m_onsetDetector, &onsinput, ons);
        bool onsdetected = *(*ons).data > 0.0;
        if (onsetdetected)
            *onsetdetected = onsdetected;
        del_fvec(ons);
    }
}

void HarmonicGroup::setOnsetParameter(string method, float threshold,float silence,float comp,int onsetbuffersize,bool adap_whitening)
{
    m_onsetMethod=method;
    m_onsetThresh=threshold;
    m_onsetSilence=silence;
    m_onsetCompression=comp;
    m_onsetBuffersize=onsetbuffersize;
    if(m_onsetDetector)
        del_aubio_onset(m_onsetDetector);
    m_onsetDetector=new_aubio_onset(m_onsetMethod.c_str(),m_onsetBuffersize,m_onsetBuffersize/2,m_samplerate);
    aubio_onset_set_threshold(m_onsetDetector,m_onsetThresh);
    // aubio_onset_set_awhitening(m_onsetDetector,adap_whitening);
    aubio_onset_set_silence(m_onsetDetector,m_onsetSilence);
    aubio_onset_set_compression(m_onsetDetector,m_onsetCompression);
    
    
}

float HarmonicGroup::getMeanEnv(int nsamples,bool* period_over)
{
    float period = 1 / m_noteClassifiers[0]->getCenterFrequency();
    int period_samples = period * m_samplerate;
    if (m_meanEnvCounter > period_samples*4)
    {
        m_meanEnv = 0;
        m_meanEnvCounter = 0;
        *period_over=true;
    }
    else
        *period_over=false;
    for (int s = 0; s < (nsamples); s++)
    {
        // if (fabs(buffer[s]) > fabs(buffer[s - 1]) && fabs(buffer[s]) > fabs(buffer[s + 1]) && fabs(buffer[s]) > 0)
        {
            float absval = fabs(m_buffer[s]);
            m_meanEnv += fabs(m_buffer[s]);
            // m_meanEnv=max(m_meanEnv,m_buffer[s]);
            // meanenv = (absval > meanenv) ? absval : meanenv;
            m_meanEnvCounter++;
        }
    }
    return m_meanEnv/m_meanEnvCounter;
}


void HarmonicGroup::process(int nsamples)
{
    memset(m_buffer, 0, nsamples * sizeof(float));
    if (audioBuffer != nullptr)
        memset(audioBuffer, 0, nsamples * sizeof(float));
    for (auto notecl : m_noteClassifiers)
        for (int s = 0; s < nsamples; s++)
            m_buffer[s] += notecl->m_buffer[s];
    bool period_over=false;
    float max=getMeanEnv(nsamples,&period_over);

    if (m_noteClassifiers[0]->getCenterFrequency() < 987.77)//&&m_noteClassifiers[0]->is_ringing)
    {
        int numringing = 0;
        vector<NoteClassifier::Ptr> ringingnotes;
        int getNumSamplesSinceLastOnset = m_noteClassifiers[0]->getNumSamplesSinceLastOnset();
        // if(numSamplesSinceLastOnset>=0)
        {
            for (auto notecl : m_noteClassifiers)
            {
                // if(notecl!=m_noteClassifiers[0])
                // if(abs(notecl->getNumSamplesSinceLastOnset()-getNumSamplesSinceLastOnset)<=10*nsamples)
                {
                    //We assume the harmonic groups are processed from lower to higher frequency 
                    //Then the higher partials must be blocked from sending midi notes themselves
                    if(notecl->is_ringing == true)
                    {
                        ringingnotes.push_back(notecl);
                        if (notecl != m_noteClassifiers[0])
                            notecl->block_midinote = false;
                    }


                }
                // else
                //     notecl->block_midinote=false;

                
                // numringing+=(notecl->getNumSamplesSinceLastOnset()>=0);
                // if(abs(notecl->getNumSamplesSinceLastOnset()-numSamplesSinceLastOnset)<=3*nsamples&&notecl->is_ringing)
                //     numringing++;
            }

            

            numringing=ringingnotes.size();
            float max = 0;
            for (int s = 0; s < nsamples; s++)
            {
                float bufabs = fabs(m_buffer[s]);
                if (bufabs > max)
                    max = bufabs;
            }

            // numringing *= m_noteClassifiers[0]->is_ringing;
            if(period_over)
            {
                if (max > 0.01 && m_noteClassifiers[0]->is_ringing && numringing >= 2) // if (numringing > 2&&max>0.1)
                {
                    if (audioBuffer != nullptr)
                        for (int s = 0; s < nsamples; s++)
                            audioBuffer[s] = m_buffer[s];
                    if (!m_oldState && !m_noteClassifiers[0]->block_midinote)
                    {
                        m_noteClassifiers[0]->sendMidiNote(nsamples, true);
                        m_oldState = true;
                        // for(auto ncl:ringingnotes)
                        //     cout<<"Freq: "<<ncl->getCenterFrequency()<<"Numsamples: "<<ncl->getNumSamplesSinceLastChangeOfState()<<endl;
                        // cout<<"partials: "<<numringing<<endl;
                        // cout<<"max: "<<max<<endl;
                    }
                }
                else
                {
                    if (m_oldState)
                    {
                        m_noteClassifiers[0]->sendMidiNote(nsamples, false);
                        m_oldState = false;
                        //  cout<<"Note off: "<<m_noteClassifiers[0]->getCenterFrequency()<<endl;
                        //  cout<<"note off partials: "<<numringing<<endl;
                    }
                }
            }
        }
    }
    else if (m_oldState)
    {
        m_noteClassifiers[0]->sendMidiNote(nsamples, false);
        m_oldState = false;
        // cout<<"Note off: "<<m_noteClassifiers[0]->getCenterFrequency()<<endl;
    }
}
