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

#include "GraphArea.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
GraphArea::GraphArea ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

GraphArea::~GraphArea()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void GraphArea::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..

    if(m_graphs&&(*m_graphs).size())
    {
        if((*m_graphs).size()==1)
        {
            auto graph=(*m_graphs)[0];
                    g.setColour(juce::Colour::fromRGB(255,255,255));
        graph->processGraph();
        g.strokePath(graph->getPath(getLocalBounds()), PathStrokeType(1.0f));
       // g.strokePath(graph->getFrequencyLine(getLocalBounds()), PathStrokeType(1.0f));
        }
        else
        {
            int c = 0;
            for (auto graph : (*m_graphs))
            {

                g.setColour(juce::Colour::fromHSV((float)c / (*m_graphs).size(), 1, 1, 1));
                c++;
                graph->processGraph();
                g.strokePath(graph->getPath(getLocalBounds()), PathStrokeType(1.0f));
                //g.strokePath(graph->getFrequencyLine(getLocalBounds()), PathStrokeType(1.0f));
                // g.drawText("hallo",Rectangle<float>(100,100,100,100),Justification::centred);
            }
        }
    }



    //[/UserPaint]
}

void GraphArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void GraphArea::drawGraphs(shared_ptr<GraphVector> graphs)
{
    m_graphs=graphs;
    repaint();
}

Rectangle<float> GraphArea::getGraphBounds()
{
    Rectangle<float> res;
    if(m_graphs)
    {
        for (auto gr : *m_graphs)
        {
            gr->processGraph();
            res = res.getUnion(gr->getBounds());
        }
    }
    return res;
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GraphArea" componentName=""
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

