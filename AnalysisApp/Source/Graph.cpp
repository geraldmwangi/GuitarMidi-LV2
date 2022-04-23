#include <Graph.hpp>

Graph::Graph(shared_ptr<NoteClassifier> notecl) : m_initialized(false), m_notecl(notecl)
{
}

Graph::~Graph()
{
}

void Graph::processGraph()
{
    m_initialized = false;
    m_path.clear();
    computeGraph();
}

void Graph::addFunctionPoint(float x, float y)
{
    if (!m_initialized)
    {
        m_minx = x;
        m_maxx = x;
        m_miny = y;
        m_maxy = y;
        m_path.startNewSubPath(x, y);
        m_initialized=true;
        
    }
    else
    {
        m_path.lineTo(x, y);
        if (x < m_minx)
            m_minx = x;
        if (x > m_maxx)
            m_maxx = x;
        if (y < m_miny)
            m_miny = y;
        if (y > m_maxy)
            m_maxy = y;
    }
}