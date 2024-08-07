#include <harmonicgroup.hpp>

HarmonicGroup::HarmonicGroup()
{
    m_oldState = false;
    m_bufferSize=256;
    m_buffer=new float[m_bufferSize];
    audioBuffer=nullptr;
}

HarmonicGroup::~HarmonicGroup()
{
    if(m_buffer)
        delete [] m_buffer;
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

void HarmonicGroup::process(int nsamples)
{

    if (m_noteClassifiers[0]->getCenterFrequency() < 987.77&&(!m_noteClassifiers[0]->block_midinote))//&&m_noteClassifiers[0]->is_ringing)
    {
        int numringing = 0;
            vector<float> diff_freq;
        float last_freq=-1;

        // if(numSamplesSinceLastOnset>=0)
        {
            for (auto notecl : m_noteClassifiers)
            {
                // if(notecl!=m_noteClassifiers[0])
                // if(abs(notecl->getNumSamplesSinceLastOnset()-getNumSamplesSinceLastOnset)<=10*nsamples)
                {
                    //We assume the harmonic groups are processed from lower to higher frequency 
                    //Then the higher partials must be blocked from sending midi notes themselves
                    // if(notecl->is_ringing == true)
                    // {
                    //     ringingnotes.push_back(notecl);
                    //     // if (notecl != m_noteClassifiers[0])
                    //     //     notecl->block_midinote = false;
                    // }
                    if(notecl->is_ringing)
                    {
                        if(last_freq==-1){
                            last_freq=notecl->getCenterFrequency();
                        }
                        else{
                            float diff=notecl->getCenterFrequency()-last_freq;
                            diff_freq.push_back(diff);
                            last_freq=notecl->getCenterFrequency();
                        }

                        numringing+=notecl->is_ringing;

                        for(int s=0;s<nsamples;s++)
                            m_buffer[s]+=notecl->m_buffer[s];
                    }

                }
                // else
                //     notecl->block_midinote=false;

                
                // numringing+=(notecl->getNumSamplesSinceLastOnset()>=0);
                // if(abs(notecl->getNumSamplesSinceLastOnset()-numSamplesSinceLastOnset)<=3*nsamples&&notecl->is_ringing)
                //     numringing++;
            }

            float max=0;
            for (int s = 0; s < nsamples; s++)
                {
                    float bufabs=fabs(m_buffer[s]);
                    if(bufabs>max)
                        max=bufabs;
                }

      
            
            // numringing *= m_noteClassifiers[0]->is_ringing;
            //if(max>0.01&&m_noteClassifiers[0]->is_ringing&&numringing >= 2)//


            if (numringing > 2&&max>0.01)
            {
                sort(diff_freq.begin(), diff_freq.end());
                float diff = diff_freq[diff_freq.size() / 2];

                if (!m_oldState && !m_noteClassifiers[0]->block_midinote && diff == m_noteClassifiers[0]->getCenterFrequency())
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
    else if (m_oldState)
    {
        m_noteClassifiers[0]->sendMidiNote(nsamples, false);
        m_oldState = false;
        // cout<<"Note off: "<<m_noteClassifiers[0]->getCenterFrequency()<<endl;
    }
}
