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
WaveFileView::WaveFileView() : m_thumbnailCache(1), m_thumbnail(512, m_formatManager, m_thumbnailCache)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    //[UserPreSize]
    //[/UserPreSize]

    setSize(600, 400);

    //[Constructor] You can add your own custom stuff here..
    m_formatManager.registerBasicFormats();
    m_thumbnail.setSource(new juce::FileInputSource(juce::File(juce::String("/home/gerald/Music/i need some_final mix more punch_r1_session.wav"))));
    m_thumbnail.addChangeListener(this);
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

    //g.fillAll(juce::Colour::fromRGB(255, 0, 0));

    //[UserPaint] Add your own custom painting code here..
    
    m_thumbnail.drawChannels(g,getBounds(),0,m_thumbnail.getTotalLength(),0.5f);
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
    std::cout<<"WaveFileView::mouseDown"<<std::endl;
    std::cout<<e.getMouseDownX()<<"x"<<e.getMouseDownScreenY()<<std::endl;
    //[/UserCode_mouseDown]
}

void WaveFileView::mouseDrag(const juce::MouseEvent &e)
{
    //[UserCode_mouseDrag] -- Add your code here...
    std::cout<<"WaveFileView::mouseDrag"<<std::endl;
    std::cout<<e.getDistanceFromDragStartX()<<"x"<<e.getDistanceFromDragStartY()<<std::endl;
    //[/UserCode_mouseDrag]
}

void WaveFileView::mouseUp(const juce::MouseEvent &e)
{
    //[UserCode_mouseUp] -- Add your code here...
    
    std::cout<<"WaveFileView::mouseUp"<<std::endl;
    //[/UserCode_mouseUp]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void WaveFileView::changeListenerCallback(juce::ChangeBroadcaster* source)
{
     //std::cout<<"WaveFileView::changeListenerCallback"<<std::endl;
     repaint();
}
//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="WaveFileView" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers=""
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
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//[EndFile] You can add extra defines here...
//[/EndFile]
