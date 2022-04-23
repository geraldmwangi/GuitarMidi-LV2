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

#include "ResponseArea.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ResponseArea::ResponseArea ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    m_filterResponseGraph.reset(new GraphArea());
    addAndMakeVisible(m_filterResponseGraph.get());
    //[/Constructor_pre]


    //[UserPreSize]
    // m_responseDrawingArea->setColour(0,juce::Colour((255<<24)|(255<<16)|(255<<8)|255));
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    m_spectrogramImage = juce::Image(juce::Image::RGB, getWidth() * 0.7, getHeight() * 0.7, true);

    m_filterResponseGraph->setSize(getWidth() * 0.7, getHeight() * 0.7);
    m_noteclassifier = make_shared<NoteClassifier>(nullptr, 48000, 82.41, 10);
    m_noteclassifier->initialize();
    m_fretboard=make_shared<FretBoard>(nullptr,48000);
    m_fretboard->initialize();
    drawSpectrum();
    //[/Constructor]
}

ResponseArea::~ResponseArea()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ResponseArea::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    {
        float x = 0.0f, y = 0.0f, width = static_cast<float> (proportionOfWidth (1.0000f)), height = static_cast<float> (proportionOfHeight (1.0000f));
        juce::Colour strokeColour = juce::Colours::white;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (strokeColour);
        g.drawRoundedRectangle (x, y, width, height, 10.000f, 5.000f);
    }

    //[UserPaint] Add your own custom painting code here..
    // g.drawImage (m_spectrogramImage, getLocalBounds().toFloat(),juce::RectanglePlacement::doNotResize);
    //[/UserPaint]
}

void ResponseArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    // m_spectrogramImage.rescaled(getWidth()*0.7, getHeight()*0.7);
    // m_spectrogramImage=juce::Image(juce::Image::RGB, getWidth()*0.7, getHeight()*0.7, true);
    // drawSpectrum();
    m_filterResponseGraph->setBounds(proportionOfWidth(0.01), proportionOfHeight(0.01), proportionOfWidth(0.9), proportionOfHeight(0.9));
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void ResponseArea::drawSpectrum()
{
    if (m_filterResponseGraph)
    {
        float minf = 70.0;
        float maxf = 500.0;

        for(auto notecl:m_fretboard->getNoteClassifiers())
        {
 

            shared_ptr<ResponseGraph> newspektrum=make_shared<ResponseGraph>(notecl);
            m_filterResponseGraph->addGraph(newspektrum);
        }
    }
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ResponseArea" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44">
    <ROUNDRECT pos="0 0 100% 100%" cornerSize="10.0" fill="solid: 2aa55e" hasStroke="1"
               stroke="5, mitered, butt" strokeColour="solid: ffffffff"/>
  </BACKGROUND>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

