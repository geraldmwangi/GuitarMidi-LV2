#include <filterbank.hpp>

GuitarMidi::FilterBank::FilterBank()
{


}

GuitarMidi::FilterBank::~FilterBank()
{
    delete [] m_filterbankbuffer.audio_buffer_2D;
}

void GuitarMidi::FilterBank::setup(map<uint, FilterRepresentation> filterreps, int samplerate)
{
    lv2_log_note(&g_logger,"Setting up filterbank\n");
    if (filterreps.size()==0)
        throw std::runtime_error("No filter representations");
    

    m_filterbankbuffer.num_filters=filterreps.size();
    m_filterbankbuffer.window_size=BUFFER_SIZE;
    
    lv2_log_note(&g_logger,"Setting up filterbank with %d filters and windowsize %d\n",m_filterbankbuffer.num_filters,m_filterbankbuffer.window_size);

    m_filterbankbuffer.audio_buffer_2D=new float[m_filterbankbuffer.num_filters*m_filterbankbuffer.window_size];
    // for(auto f:filterreps){
    //     shared_ptr<Filter>  filter=make_shared<Filter>(f.second,samplerate);

    //     filter->setOutput((m_filterbankbuffer.audio_buffer_2D+f.first*m_filterbankbuffer.window_size));
    //     m_filters.insert(make_pair(f.first,filter));

    // }
}

    /**
    * The FilterBank class manages a collection of Filter objects and is set up based on a provided map of filter representations.
    * It contains one 2D audio buffer that holds the output of all the filters in the bank in which each row corresponds to the output of a single filter.
    * This buffer is then used in noteinferencer to infer the played notes based on the output of the filters in the filter bank.
    *  * Difference equation direct form II implementation of the filters in the filter bank:
 * d1[n]=v[n-1]
 * d2[n]=v[n-2]
 *  v[n] =         x[n] - (a1/a0)*d1[n] - (a2/a0)*d2[n]
 *  y(n) = (b0/a0)*v[n] + (b1/a0)*d1[n] + (b2/a0)*d2[n]
    */
void GuitarMidi::FilterBank::process(int nsamples)
{
    for(int f=0;f<NUM_NOTES*NUM_HARMONICS;f++){
        //n=0
        float d1=m_d1[f];
        float d2=m_d2[f];
        float v;
        float a1=m_a1[f];
        float a2=m_a2[f];
        float b0=m_b0[f];
        float b1=m_b1[f];
        float b2=m_b2[f];
        float* output=(m_filterbankbuffer.audio_buffer_2D+f*m_filterbankbuffer.window_size);
        for(int s=0;s<nsamples;s++){
            v =  m_input[s] - a1*d1 - a2*d2 + 1e-20f;
            output[s]= b0*v + b1*d1 + b2*d2 + 1e-20f;
            d2=d1;
            d1=v;
           
        }
        m_d1[f]=d1;
        m_d2[f]=d2;
    }
}
