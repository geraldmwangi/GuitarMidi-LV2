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
#include <fretboard.hpp>
#include <PlotArea.h>
#include <ResponseGraph.hpp>
//[/Headers]

//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ResponseArea : public juce::Component
{
public:
  //==============================================================================
  ResponseArea(shared_ptr<FretBoard> fretboard);
  ~ResponseArea() override;

  //==============================================================================
  //[UserMethods]     -- You can add your own custom methods in this section.
  void drawGraphs(int graph_index)
  {
    if (m_filterResponseGraph)
    {
      shared_ptr<GraphVector> graphs = make_shared<GraphVector>();
      if (graph_index == ALL_NOTECLS)
      {

        for (auto notecl : m_fretboard->getNoteClassifiers())
        {

          shared_ptr<ResponseGraph> newspektrum = make_shared<ResponseGraph>(notecl);
          graphs->push_back(newspektrum);
        }
      }
      else
      {
        auto notecl = m_fretboard->getNoteClassifiers()[graph_index];
        shared_ptr<ResponseGraph> newspektrum = make_shared<ResponseGraph>(notecl);
        graphs->push_back(newspektrum);
      }
      m_filterResponseGraph->drawGraphs(graphs);
    }
  }
  //[/UserMethods]

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  //[UserVariables]   -- You can add your own custom variables in this section.
  juce::Image m_spectrogramImage;
  std::shared_ptr<NoteClassifier> m_noteclassifier;
  std::shared_ptr<FretBoard> m_fretboard;
  std::unique_ptr<PlotArea> m_filterResponseGraph;
  //[/UserVariables]

  //==============================================================================

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResponseArea)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
