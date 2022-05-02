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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "WaveFileView.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
WaveFileView::WaveFileView()
    : m_thumbnailCache(1), m_thumbnail(512, m_formatManager, m_thumbnailCache)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    //[UserPreSize]
    //[/UserPreSize]

    setSize(600, 400);

    //[Constructor] You can add your own custom stuff here..
    m_formatManager.registerBasicFormats();
    juce::String file = juce::String("/home/gerald/Music/guitarmidi/A-Chord-lespaul.wav");
    auto *reader = m_formatManager.createReaderFor(file);
    std::cout << "Loading file: " << file << std::endl;
    std::cout << "Samplerate: " << reader->sampleRate << " Hz" << std::endl;
    std::cout << "Length: " << reader->lengthInSamples << " samples" << std::endl;
    m_buffer.setSize(1, reader->lengthInSamples);
    reader->read(&m_buffer, 0, reader->lengthInSamples, 0, true, false);
    m_readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

    m_transportSource.setSource(m_readerSource.get(), 0, nullptr, reader->sampleRate);
    m_transportSource.prepareToPlay(256, 48000);
    m_thumbnail.setSource(new juce::FileInputSource(juce::File(file)));
    m_thumbnail.addChangeListener(this);
    m_linePositionX = -1;
    m_audioSlice.setSize(1, 8192);
    m_lastPosSample = 0;
    //[/Constructor]
}

WaveFileView::~WaveFileView()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void WaveFileView::paint(juce::Graphics &g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..

    m_thumbnail.drawChannels(g, getLocalBounds(), 0, m_thumbnail.getTotalLength(), 0.5f);
    g.setColour(juce::Colour::fromRGB(255, 255, 255));
    int width = ((float)m_audioSlice.getNumSamples()) / m_buffer.getNumSamples() * getBounds().getWidth();

    if (m_linePositionX >= 0)
        if (width == 0)
            g.drawLine(m_linePositionX, 0, m_linePositionX, getBounds().getHeight());
        else
            g.drawRect(m_linePositionX, 0, width, getBounds().getHeight());
    //[/UserPaint]
}

void WaveFileView::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..

    //[/UserResized]
}

void WaveFileView::mouseMove(const juce::MouseEvent &e)
{
    //[UserCode_mouseMove] -- Add your code here...
    //[/UserCode_mouseMove]
}

void WaveFileView::mouseEnter(const juce::MouseEvent &e)
{
    //[UserCode_mouseEnter] -- Add your code here...
    //[/UserCode_mouseEnter]
}

void WaveFileView::mouseExit(const juce::MouseEvent &e)
{
    //[UserCode_mouseExit] -- Add your code here...
    //[/UserCode_mouseExit]
}

void WaveFileView::mouseDown(const juce::MouseEvent &e)
{
    //[UserCode_mouseDown] -- Add your code here...
    std::cout << "WaveFileView::mouseDown" << std::endl;
    std::cout << e.getMouseDownX() << "x" << e.getMouseDownScreenY() << std::endl;
    m_offsetX = e.getMouseDownX();
    m_linePositionX = m_offsetX;

    double pos_secs = ((float)m_linePositionX) / (getLocalBounds().getWidth()) * m_transportSource.getLengthInSeconds();
    double pos_sample = ((float)m_linePositionX) / (getLocalBounds().getWidth()) * m_buffer.getNumSamples() - 1;
    pos_sample = (pos_sample >= m_buffer.getNumSamples()) ? (m_buffer.getNumSamples() - 1) : pos_sample;
    pos_sample = (pos_sample < 0) ? 0 : pos_sample;

    std::cout << "Line position: " << pos_secs << " s"
              << ", " << pos_sample << " samples" << std::endl;

    // int dir=(pos_sample-m_lastPosSample)/std::abs(pos_sample-m_lastPosSample);
    // for(int p=m_lastPosSample;dir*p<dir*pos_sample;p+=dir)
    // {
    //     m_audioSlice.copyFrom(0, 0, m_buffer, 0, p, 512);
    //     sendSynchronousChangeMessage();
    // }
    // m_lastPosSample=pos_sample;

    for(int i=0;i<1;i++)
    {
        m_audioSlice.copyFrom(0, 0, m_buffer, 0, pos_sample+i*m_audioSlice.getNumSamples(), m_audioSlice.getNumSamples());
        juce::dsp::WindowingFunction<float> window(m_audioSlice.getNumSamples(), juce::dsp::WindowingFunction<float>::hamming);
        // window.multiplyWithWindowingTable(*m_audioSlice.getArrayOfWritePointers(), m_audioSlice.getNumSamples());
        sendSynchronousChangeMessage();
        repaint();
    }
    //[/UserCode_mouseDown]
}

void WaveFileView::mouseDrag(const juce::MouseEvent &e)
{
    //[UserCode_mouseDrag] -- Add your code here...
    std::cout << "WaveFileView::mouseDrag" << std::endl;
    std::cout << e.getDistanceFromDragStartX() << "x" << e.getDistanceFromDragStartY() << std::endl;
    if (e.mouseWasDraggedSinceMouseDown())
        m_linePositionX = m_offsetX + e.getDistanceFromDragStartX();
    double pos_secs = ((float)m_linePositionX) / (getLocalBounds().getWidth()) * m_transportSource.getLengthInSeconds();
    double pos_sample = ((float)m_linePositionX) / (getLocalBounds().getWidth()) * m_buffer.getNumSamples() - 1;
    pos_sample = (pos_sample >= m_buffer.getNumSamples()) ? (m_buffer.getNumSamples() - 1) : pos_sample;
    pos_sample = (pos_sample < 0) ? 0 : pos_sample;

    std::cout << "Line position: " << pos_secs << " s" << std::endl;
    std::cout << "Line position: " << pos_secs << " s"
              << ", " << pos_sample << " samples" << std::endl;

    // int dir=(pos_sample-m_lastPosSample)/std::abs(pos_sample-m_lastPosSample);
    // for(int p=m_lastPosSample;dir*p<dir*pos_sample;p+=dir)
    // {
    //     m_audioSlice.copyFrom(0, 0, m_buffer, 0, p, 512);
    //     sendSynchronousChangeMessage();
    // }
    // m_lastPosSample=pos_sample;
     for(int i=0;i<1;i++)
    {
        m_audioSlice.copyFrom(0, 0, m_buffer, 0, pos_sample+i*m_audioSlice.getNumSamples(), m_audioSlice.getNumSamples());
        juce::dsp::WindowingFunction<float> window(m_audioSlice.getNumSamples(), juce::dsp::WindowingFunction<float>::hamming);
        // window.multiplyWithWindowingTable(*m_audioSlice.getArrayOfWritePointers(), m_audioSlice.getNumSamples());
        sendSynchronousChangeMessage();
        repaint();
    }
    //[/UserCode_mouseDrag]
}

void WaveFileView::mouseUp(const juce::MouseEvent &e)
{
    //[UserCode_mouseUp] -- Add your code here...

    std::cout << "WaveFileView::mouseUp" << std::endl;
    // m_linePositionX = -1;
    repaint();
    //[/UserCode_mouseUp]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void WaveFileView::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    std::cout << "WaveFileView::changeListenerCallback" << std::endl;

    repaint();
}

juce::AudioSampleBuffer WaveFileView::getCurrentAudioSlice()
{
    juce::AudioSampleBuffer res;
    res.makeCopyOf(m_audioSlice);
    return res;
}
//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="WaveFileView" componentName=""
                 parentClasses="public juce::Component, public juce::ChangeListener, public juce::ChangeBroadcaster"
                 constructorParams="" variableInitialisers=" m_thumbnailCache(1), m_thumbnail(512, m_formatManager, m_thumbnailCache)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <METHODS>
    <METHOD name="mouseMove (const juce::MouseEvent&amp; e)"/>
    <METHOD name="mouseEnter (const juce::MouseEvent&amp; e)"/>
    <METHOD name="mouseExit (const juce::MouseEvent&amp; e)"/>
    <METHOD name="mouseDown (const juce::MouseEvent&amp; e)"/>
    <METHOD name="mouseDrag (const juce::MouseEvent&amp; e)"/>
    <METHOD name="mouseUp (const juce::MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//[EndFile] You can add extra defines here...
//[/EndFile]
