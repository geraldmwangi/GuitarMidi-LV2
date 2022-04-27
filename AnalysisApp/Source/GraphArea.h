/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 6.1.6

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include <JuceHeader.h>
#include <vector>
#include <Graph.hpp>
#define ALL_NOTECLS -1
typedef std::vector<shared_ptr<Graph>> GraphVector;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class GraphArea  : public juce::Component
{
public:
    //==============================================================================
    GraphArea ();
    ~GraphArea() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
  void drawGraphs(shared_ptr<GraphVector> graph);
  Rectangle<float> getGraphBounds();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
  shared_ptr<GraphVector> m_graphs;
    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphArea)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

