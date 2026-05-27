#include <filterbank.hpp>
#include <pmmintrin.h>  
#include <xmmintrin.h> 
// based on https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
GuitarMidi::FilterBank::FilterBank()
{

    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
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
    float q=Q_FACTOR;
    for(auto f:filterreps){
        float center_freq=f.second.center_freq;
        int filter_id=f.second.filter_id;
        // comptue biquad bandpasscoefficients for the filter based on the center frequency and q factor
        float omega=2*M_PI*center_freq/samplerate;
        float alpha=sin(omega)/(2*q);
        float bw=center_freq/q;
        float a0=1.0+alpha;
        float a1= -2*cos(2*M_PI*center_freq/samplerate);
        float a2=1.0-alpha;
        float b0=q*alpha;
        float b1=0.0;
        float b2=-q*alpha;
        m_a0[filter_id]=a0/a0;
        m_a1[filter_id]=a1/a0;
        m_a2[filter_id]=a2/a0;
        m_b0[filter_id]=b0/a0;
        m_b1[filter_id]=b1/a0;
        m_b2[filter_id]=b2/a0;

    }
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
        float* __restrict output=(m_filterbankbuffer.audio_buffer_2D+f*m_filterbankbuffer.window_size);
        for(int s=0;s<nsamples;s++){
            v =  m_input[s] - a1*d1 - a2*d2;
           // output[s]=fabs (b0*v + b1*d1 + b2*d2); b1=0 for bandpass filters
            output[s]=fabs (b0*v + b2*d2);
            d2=d1;
            d1=v;
           
        }
        m_d1[f]=d1;
        m_d2[f]=d2;
    }
}
