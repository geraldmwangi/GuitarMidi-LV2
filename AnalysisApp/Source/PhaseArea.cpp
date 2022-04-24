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

#include "PhaseArea.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PhaseArea::PhaseArea (shared_ptr<FretBoard> fretboard)
{
    //[Constructor_pre] You can add your own custom stuff here..
    m_filterPhaseGraph.reset(new PlotArea());
    addAndMakeVisible(m_filterPhaseGraph.get());
    //[/Constructor_pre]


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    m_filterPhaseGraph->setSize(getWidth() * 0.7, getHeight() * 0.7);
    m_fretboard =fretboard;
    drawPhaseDiagram();
    //[/Constructor]
}

PhaseArea::~PhaseArea()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PhaseArea::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff473737));

    {
        float x = static_cast<float> (-1), y = static_cast<float> (-1), width = static_cast<float> (proportionOfWidth (1.0000f)), height = static_cast<float> (proportionOfHeight (1.0000f));
        juce::Colour strokeColour = juce::Colours::white;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (strokeColour);
        g.drawRoundedRectangle (x, y, width, height, 10.000f, 5.000f);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PhaseArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    m_filterPhaseGraph->setBounds(proportionOfWidth(0.01), proportionOfHeight(0.01), proportionOfWidth(0.9), proportionOfHeight(0.9));
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void PhaseArea::drawPhaseDiagram()
{
    if (m_filterPhaseGraph)
    {
        float minf = 70.0;
        float maxf = 500.0;

        for (auto notecl : m_fretboard->getNoteClassifiers())
        {

            shared_ptr<PhaseGraph> newspektrum = make_shared<PhaseGraph>(notecl);
            m_filterPhaseGraph->addGraph(newspektrum);
            // break;
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

<JUCER_COMPONENT documentType="Component" className="PhaseArea" componentName=""
                 parentClasses="public juce::Component" constructorParams="shared_ptr&lt;FretBoard&gt; fretboard"
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff473737">
    <ROUNDRECT pos="-1 -1 100% 100%" cornerSize="10.0" fill="solid: ffffff"
               hasStroke="1" stroke="5, mitered, butt" strokeColour="solid: ffffffff"/>
  </BACKGROUND>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

