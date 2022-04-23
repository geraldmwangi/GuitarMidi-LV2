#include <PhaseGraph.hpp>
#include <math.h>

PhaseGraph::PhaseGraph(shared_ptr<NoteClassifier> notecl) : Graph(notecl)
{
}

PhaseGraph::~PhaseGraph()
{
}

void PhaseGraph::computeGraph()
{
    float minf = 70.0;
    float maxf = 500.0;

    for (float f = minf; f < maxf; f += 1.0)
    {

        Dsp::complex_t c = m_notecl->filterResponse(f);
        float phase = 0;
        if(fabs(c)>0)
        {
            if (c.real() > 0)
                phase = atan2(c.imag(), c.real());
            else if (c.real() < 0 && c.imag() >= 0)
                phase = atan2(c.imag(), c.real()) + M_PI;
            else if (c.real() < 0 && c.imag() < 0)
                phase = atan2(c.imag(), c.real()) - M_PI;
            else if (c.real() == 0 && c.imag() >= 0)
                phase = M_PI / 2;
            else if (c.real() == 0 && c.imag() < 0)
                phase = -M_PI / 2;

            // float resp = float(std::abs(c));
            // float re=(resp==0)?0:c.real()/resp;

            // float phase=std::acos(re);

            addFunctionPoint(f, 1.0 - phase);
        }
    }
}