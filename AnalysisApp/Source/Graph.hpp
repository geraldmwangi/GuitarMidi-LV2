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
    
   

    float m_minx;
    float m_maxx;
    float m_miny;
    float m_maxy;
    

    protected:
    Path m_path;
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

    virtual void drawGraph(juce::Graphics& g, juce::Rectangle<int> bounds )
    {
        g.strokePath(this->getPath(bounds), PathStrokeType(1.0f));
    }

    Path getFrequencyLine(juce::Rectangle<int> bounds);

    Path getPath()
    {
        return m_path;
    }

    Rectangle<float> getBounds()
    {
        return m_path.getBounds();
    }
};