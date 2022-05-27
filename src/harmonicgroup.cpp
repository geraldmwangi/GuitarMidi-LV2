#include <harmonicgroup.hpp>

HarmonicGroup::HarmonicGroup()
{
    m_oldState = false;
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
    if (m_noteClassifiers[0]->getCenterFrequency() < 987.77&&m_noteClassifiers[0]->is_ringing)
    {
        int numringing = 0;
        int numSamplesSinceLastChangeOfState = m_noteClassifiers[0]->getNumSamplesSinceLastChangeOfState();
        // if(numSamplesSinceLastOnset>=0)
        {
            for (auto notecl : m_noteClassifiers)
            {
                // if(notecl!=m_noteClassifiers[0])
                //if(abs(notecl->getNumSamplesSinceLastChangeOfState()-numSamplesSinceLastChangeOfState)<=3*nsamples)
                {
                    numringing += (notecl->is_ringing == true);
                    if(notecl!=m_noteClassifiers[0])
                        notecl->block_midinote=true;
                }
                
                // numringing+=(notecl->getNumSamplesSinceLastOnset()>=0);
                // if(abs(notecl->getNumSamplesSinceLastOnset()-numSamplesSinceLastOnset)<=3*nsamples&&notecl->is_ringing)
                //     numringing++;
            }
            
            numringing *= m_noteClassifiers[0]->is_ringing;
            if (numringing > 1)
            {
                if (!m_oldState&&m_noteClassifiers[0]->is_ringing&&!m_noteClassifiers[0]->block_midinote)
                {
                    m_noteClassifiers[0]->sendMidiNote(nsamples, true);
                    m_oldState = true;
                    cout<<"Freq: "<<m_noteClassifiers[0]->getCenterFrequency()<<"Numsamples: "<<m_noteClassifiers[0]->getNumSamplesSinceLastChangeOfState()<<endl;
                    cout<<"partials: "<<numringing<<endl;
                }
            }
            else
            {
                if (m_oldState)
                {
                    m_noteClassifiers[0]->sendMidiNote(nsamples, false);
                    m_oldState = false;
                    // cout<<"Note off: "<<m_noteClassifiers[0]->getCenterFrequency()<<endl;
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
