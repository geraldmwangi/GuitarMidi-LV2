#include <Graph.hpp>

Graph::Graph() : m_initialized(false)
{
}

Graph::~Graph()
{
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