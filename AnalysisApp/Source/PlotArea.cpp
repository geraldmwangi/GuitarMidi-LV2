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

#include "PlotArea.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PlotArea::PlotArea()
{
    //[Constructor_pre] You can add your own custom stuff here..
    m_graphArea.reset(new GraphArea());
    addAndMakeVisible(m_graphArea.get());
    //[/Constructor_pre]

    //[UserPreSize]
    //[/UserPreSize]

    setSize(600, 400);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

PlotArea::~PlotArea()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PlotArea::paint(juce::Graphics &g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll(juce::Colour(0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    Rectangle<float> valueBounds = m_graphArea->getGraphBounds();
    g.setColour(juce::Colour::fromRGB(255, 255, 255));
    //Line<int> ordinate(getBounds().getTopLeft(), getBounds().getBottomLeft());
     Line<int> ordinate(proportionOfWidth(0.1),proportionOfHeight(0.0),proportionOfWidth(0.1),proportionOfHeight(0.9));
    g.drawLine(ordinate.getStartX(), ordinate.getStartY(), ordinate.getEndX(), ordinate.getEndY());

    int numticks = 10;
    for (int i = 0; i < numticks; i++)
    {
        float y = (ordinate.getEndY() - ordinate.getStartY()) * ((float)i) / numticks + ordinate.getStartY();
        float valy = (valueBounds.getTopLeft().getY() - valueBounds.getBottomLeft().getY()) * ((float)i) / numticks + valueBounds.getBottomLeft().getY();
        
        g.drawText(String(valy), ordinate.getStartX()-50, y, 50, 100, Justification::topLeft);
    }

    Line<int> abscisse(proportionOfWidth(0.1),proportionOfHeight(0.9),proportionOfWidth(0.9),proportionOfHeight(0.9));
    g.drawLine(abscisse.getStartX(), abscisse.getStartY(), abscisse.getEndX(), abscisse.getEndY());

    for (int i = 0; i < numticks; i++)
    {
        float x = (abscisse.getEndX() - abscisse.getStartX()) * ((float)i) / numticks + abscisse.getStartX();
        float valx = (valueBounds.getBottomRight().getX() - valueBounds.getBottomLeft().getX()) * ((float)i) / numticks + valueBounds.getBottomLeft().getX();
        
        g.drawText(String(valx), x,abscisse.getStartY(), 50, 100, Justification::topLeft);
    }
    //[/UserPaint]
}

void PlotArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    m_graphArea->setBounds(proportionOfWidth(0.1), proportionOfHeight(0.0), proportionOfWidth(0.9), proportionOfHeight(0.9));
    //[/UserResized]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PlotArea" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//[EndFile] You can add extra defines here...
//[/EndFile]
