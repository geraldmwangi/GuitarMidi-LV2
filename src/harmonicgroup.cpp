#include <harmonicgroup.hpp>

HarmonicGroup::HarmonicGroup(float fundamentalFreq)
{
    m_fundamentalFreq=fundamentalFreq;
}

void HarmonicGroup::addNoteClassifier(shared_ptr<NoteClassifier> notecl)
{
    m_noteClassifiers.push_back(notecl);
}

void HarmonicGroup::process(int nsamples)
{
    int numringing=0;
    for(auto notecl:m_noteClassifiers)
    {
        numringing+=notecl->getNoteOnOffState();

    }

    if(numringing>1)
    {
        m_noteClassifiers[0]->sendMidiNote(nsamples);
    }
    
}
