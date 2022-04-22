#pragma once
#include <JuceHeader.h>

using namespace juce;

/**
 * @brief Graph is a class which contains (x,y(x)) points to draw in a GrapgArea
 *
 */
class Graph
{
private:
    Path m_path;

    float m_minx;
    float m_maxx;
    float m_miny;
    float m_maxy;

public:
    Graph();
    virtual ~Graph();

    void addFunctionPoint(float x,float y);
    Path& getPath(juce::Rectangle<int> bounds)
    {
   
        m_path.scaleToFit((float)bounds.getX(),(float)bounds.getY(),(float)bounds.getWidth(),(float)bounds.getHeight(),false);
        return m_path;
    }
};