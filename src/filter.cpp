#include <filter.hpp>
namespace GuitarMidi
{
    Filter::Filter(FilterRepresentation filter_rep, float samplerate, float q, float passbandatten)
    {
        m_filter_rep=filter_rep;
        m_passbandatten = passbandatten;
        m_q = q;
        m_samplerate = samplerate;

        m_bufferSize = BUFFER_SIZE;
        m_buffer = nullptr;
        m_output = nullptr;
        initialize();
    }

    void Filter::setFilterParameters(float q, float passbandatten, int order)
    {
        m_q = q;
        m_passbandatten = passbandatten;
        m_order = order;

        m_filter.reset();
        float bw=m_filter_rep.center_freq/m_q;

#ifdef USE_ELLIPTIC_FILTERS
        printf("USE_ELLIPTIC_FILTERS\n");
        m_filter.setup(m_order, m_samplerate, m_filter_rep.center_freq, bw, m_passbandatten, 18.0);
#else
        //printf("DO NOT USE_ELLIPTIC_FILTERS\n");
        m_filter.setup(m_order, m_samplerate, m_filter_rep.center_freq, bw);
#endif
    }

    Filter::~Filter()
    {
        m_filter.reset();
        if (m_buffer)
            delete[] m_buffer;
    }

    void Filter::initialize()
    {
      
        setFilterParameters(m_q, m_passbandatten);

        if (m_bufferSize)
            m_buffer = new float[m_bufferSize];
    }

    void Filter::process(int nsamples)
    {
        memcpy(m_buffer, m_input, nsamples * sizeof(float));

        m_filter.process(nsamples, &m_buffer);
        // m_filter.process(nsamples, &m_buffer);
        for (int b=0;b<m_bufferSize;b++)
            m_buffer[b]=fabs(m_buffer[b]);

        if (m_output != nullptr)
        {
            memcpy(m_output, m_buffer, nsamples * sizeof(float));
        }
    }
}