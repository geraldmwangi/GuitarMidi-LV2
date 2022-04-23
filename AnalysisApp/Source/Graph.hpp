#pragma once
#include <JuceHeader.h>
#include <noteclassifier.hpp>

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
    

    protected:
    bool m_initialized;
    shared_ptr<NoteClassifier> m_notecl;
    virtual void computeGraph()=0;

public:
    Graph(shared_ptr<NoteClassifier> notecl);
    virtual ~Graph();

    void processGraph();

    void addFunctionPoint(float x,float y);
    Path getPath(juce::Rectangle<int> bounds)
    {
        Path scaledPath=m_path;
        scaledPath.scaleToFit((float)bounds.getX(),(float)bounds.getY(),(float)bounds.getWidth(),(float)bounds.getHeight(),false);
        return scaledPath;
    }

    Path getPath()
    {
        return m_path;
    }

    Rectangle<float> getBounds()
    {
        return m_path.getBounds();
    }
};