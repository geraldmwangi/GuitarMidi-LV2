#include <harmonicgroup.hpp>

HarmonicGroup::HarmonicGroup()
{
    m_oldState=false;
}

void HarmonicGroup::addNoteClassifier(shared_ptr<NoteClassifier> notecl)
{
    if(m_noteClassifiers.size()==0)
        m_noteClassifiers.push_back(notecl); //Add fundamental
    else
    {
        for(int n=2;n<=12;n++)
        {
            float min=m_noteClassifiers[0]->getCenterFrequency()*n-1;
            float max=m_noteClassifiers[0]->getCenterFrequency()*n+1;
            float f=notecl->getCenterFrequency();
            if(f>=min&&f<=max)
                m_noteClassifiers.push_back(notecl);
        }
    }
}

void HarmonicGroup::process(int nsamples)
{
    int numringing=0;
    int numSamplesSinceLastOnset=m_noteClassifiers[0]->getNumSamplesSinceLastOnset();
    //if(numSamplesSinceLastOnset>=0)
    {
        for (auto notecl : m_noteClassifiers)
        {
            // if(notecl!=m_noteClassifiers[0])
            numringing += (notecl->is_ringing == true);
            // numringing+=(notecl->getNumSamplesSinceLastOnset()>=0);
            // if(abs(notecl->getNumSamplesSinceLastOnset()-numSamplesSinceLastOnset)<=3*nsamples&&notecl->is_ringing)
            //     numringing++;
        }
        numringing*=m_noteClassifiers[0]->is_ringing;
        if (numringing > 1)
        {
            if (!m_oldState)
            {
                m_noteClassifiers[0]->sendMidiNote(nsamples, true);
                m_oldState = true;
                // cout<<"Note on: "<<m_noteClassifiers[0]->getCenterFrequency()<<endl;
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