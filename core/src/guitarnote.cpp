#include <guitarmidi/guitarnote.hpp>
#include <memory>
namespace GuitarMidi
{
    GuitarNote::GuitarNote(int note_id, float centerfreq)
    {
        for (int harmonic=1;harmonic<=NUM_HARMONICS;harmonic++){
            float f_mult=centerfreq*harmonic;
            int filter_id = note_id* NUM_HARMONICS + harmonic-1;
            FilterRepresentation frep;
            frep.center_freq=f_mult;
            frep.filter_id=filter_id;
            m_filters.push_back(frep);
            // auto filter=std::make_shared<Filter>(fret,string_id,i-1, samplerate,f_mult, bandwidth,passbandatten);
        }
    }
    GuitarNote::~GuitarNote()
    {
    }
}
