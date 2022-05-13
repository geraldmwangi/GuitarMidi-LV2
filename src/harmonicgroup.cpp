#include <harmonicgroup.hpp>

HarmonicGroup::HarmonicGroup()
{
}

void HarmonicGroup::addNoteClassifier(shared_ptr<NoteClassifier> notecl)
{
    if(m_noteClassifiers.size()==0)
        m_noteClassifiers.push_back(notecl); //Add fundamental
    else
    {
        for(int n=2;n<=4;n++)
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
    for(auto notecl:m_noteClassifiers)
    {
        numringing+=(notecl->getNoteOnOffState()==true);

    }

    if(numringing>1)
    {
        // m_noteClassifiers[0]->setNoteOnOffState(true);
        m_noteClassifiers[0]->sendMidiNote(nsamples);
    }
    // else
    // {
    //     m_noteClassifiers[0]->setNoteOnOffState(false);
    //     m_noteClassifiers[0]->sendMidiNote(nsamples);        
    // }
    
}
