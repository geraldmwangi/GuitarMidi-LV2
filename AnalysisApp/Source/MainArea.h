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
#include <PlotArea.h>
#include <ResponseGraph.hpp>
#include <FilteredAudioGraph.hpp>
#include <PhaseGraph.hpp>
#include <fretboard.hpp>
#include <WaveFileView.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MainArea  : public juce::Component,
                  public juce::ChangeListener,
                  public juce::ComboBox::Listener,
                  public juce::Slider::Listener
{
public:
    //==============================================================================
    MainArea ();
    ~MainArea() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    virtual void changeListenerCallback (ChangeBroadcaster* source);
    void drawGraphs();
    void setOnsetDetectors();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void sliderValueChanged (juce::Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    unique_ptr<PlotArea>    m_responseArea;
    unique_ptr<PlotArea>    m_meanResponseAudioArea;
    unique_ptr<PlotArea>    m_responseAudioArea;
    unique_ptr<PlotArea>     m_phaseArea;
    shared_ptr<FretBoard>     m_fretboard;
    int m_currentNoteCl;
    unique_ptr<WaveFileView> m_waveFileView;
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<juce::TabbedComponent> m_phaseResponseTab;
    std::unique_ptr<juce::GroupComponent> m_controlsArea;
    std::unique_ptr<juce::ComboBox> m_noteClSelector;
    std::unique_ptr<juce::Slider> m_bandwidthInput;
    std::unique_ptr<juce::GroupComponent> m_waveFileGroup;
    std::unique_ptr<juce::Slider> m_orderInput;
    std::unique_ptr<juce::ComboBox> m_onsetMethod;
    std::unique_ptr<juce::Slider> m_onsetthreshold;
    std::unique_ptr<juce::Slider> m_onsetsilence;
    std::unique_ptr<juce::Slider> m_onsetcomp;
    std::unique_ptr<juce::ComboBox> m_onsetBufferSize;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainArea)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

