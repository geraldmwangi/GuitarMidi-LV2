#include <ResponseGraph.hpp>

ResponseGraph::ResponseGraph(shared_ptr<NoteClassifier> notecl) : Graph(notecl)
{
}

ResponseGraph::~ResponseGraph()
{
}

void ResponseGraph::computeGraph()
{
    float minf = 70.0;
    float maxf = 500.0;
    m_initialized = false;

    for (float f = minf; f < maxf; f += 1.0)
    {

        Dsp::complex_t c = m_notecl->filterResponse(f);
        float resp = float(std::abs(c));

        addFunctionPoint(f, 1.0 - resp);
    }
}