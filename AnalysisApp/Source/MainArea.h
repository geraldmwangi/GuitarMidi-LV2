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
#include <ResponseArea.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MainArea  : public juce::Component,
                  public juce::ComboBox::Listener
{
public:
    //==============================================================================
    MainArea ();
    ~MainArea() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    unique_ptr<ResponseArea>    m_responseArea;
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<juce::TabbedComponent> m_phaseResponseTab;
    std::unique_ptr<juce::GroupComponent> m_controlsArea;
    std::unique_ptr<juce::ComboBox> m_noteClSelector;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainArea)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
