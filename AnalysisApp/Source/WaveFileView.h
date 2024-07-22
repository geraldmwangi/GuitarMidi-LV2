/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 8.0.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) - Raw Material Software Limited.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include <JuceHeader.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class WaveFileView  : public juce::Component,
                      public juce::ChangeListener,
                      public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    WaveFileView ();
    ~WaveFileView() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
  virtual void changeListenerCallback(juce::ChangeBroadcaster *source);
  juce::AudioSampleBuffer getCurrentAudioSlice();
  juce::AudioSampleBuffer getAudioBuffer();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
  juce::AudioThumbnail m_thumbnail;
  juce::AudioFormatManager m_formatManager;
  juce::AudioThumbnailCache m_thumbnailCache; // [3]
  std::unique_ptr<juce::AudioFormatReaderSource> m_readerSource;
  juce::AudioTransportSource m_transportSource;
  juce::AudioSampleBuffer m_audioSlice;
  int m_linePositionX;
  int m_lastPosSample;
  int m_offsetX;
  juce::AudioSampleBuffer m_buffer;
    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveFileView)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

