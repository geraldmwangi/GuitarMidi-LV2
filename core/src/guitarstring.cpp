#include <guitarmidi/guitarstring.hpp>

namespace GuitarMidi
{
    GuitarString::GuitarString(map<int,int> note_freqs)
    {
        for (auto nf: note_freqs){
            m_notes.push_back(GuitarNote(nf.first,nf.second));
        }
    }
}