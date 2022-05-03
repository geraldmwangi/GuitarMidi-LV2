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
#include <GraphArea.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PlotArea  : public juce::Component
{
public:
    //==============================================================================
    PlotArea ();
    ~PlotArea() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
  void drawGraphs(shared_ptr<GraphVector> graph)
  {
    m_graphArea->drawGraphs(graph);
    repaint();
  }

    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
  std::unique_ptr<GraphArea> m_graphArea;
  int m_linePositionX;
  int m_linePositionXOffset;
    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlotArea)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

