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
    m_linePositionX=-1;
    m_linePositionXOffset=-1;
    m_linePositionY=-1;
    m_linePositionYOffset=-1;
    //[/Constructor]
}

GraphArea::~GraphArea()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
    m_linePositionX = -1;

    //[/Destructor]
}

//==============================================================================
void GraphArea::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..

    if (m_graphs && (*m_graphs).size())
    {
        if ((*m_graphs).size() == 1)
        {
            auto graph = (*m_graphs)[0];
            g.setColour(juce::Colour::fromRGB(255, 255, 255));
            graph->processGraph();
            // g.strokePath(graph->getPath(getLocalBounds()), PathStrokeType(1.0f));
            graph->drawGraph(g, getLocalBounds());
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
                graph->drawGraph(g, getLocalBounds());
                // g.strokePath(graph->getPath(getLocalBounds()), PathStrokeType(1.0f));
                // g.strokePath(graph->getFrequencyLine(getLocalBounds()), PathStrokeType(1.0f));
                //  g.drawText("hallo",Rectangle<float>(100,100,100,100),Justification::centred);
            }
        }
    }
    // if (m_linePositionX >= 0)
    // {
    //     g.drawLine(m_linePositionX, 0, m_linePositionX, getHeight());
    // }

    //[/UserPaint]
}

void GraphArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..

    //[/UserResized]
}

void GraphArea::mouseDown (const juce::MouseEvent& e)
{
    //[UserCode_mouseDown] -- Add your code here...
    m_linePositionX = e.getMouseDownX();
    m_linePositionXOffset = m_linePositionX;
    m_linePositionY=e.getMouseDownY();
    m_linePositionYOffset=m_linePositionY;
    m_lastBoundsRelativToParent=getBounds();

    repaint();
    //[/UserCode_mouseDown]
}

void GraphArea::mouseDrag (const juce::MouseEvent& e)
{
    //[UserCode_mouseDrag] -- Add your code here...
    if(e.getDistanceFromDragStartX()>e.getDistanceFromDragStartY())
        m_linePositionX = m_linePositionXOffset + e.getDistanceFromDragStartX();
    if(abs(e.getDistanceFromDragStartY()))
    {
        float s=(((float)e.getDistanceFromDragStartY())/m_lastBoundsRelativToParent.getHeight());

         s=1+s;//pow(2,s);



        //The origin is the top left of this object in the frame of the parent
        auto lp=m_lastBoundsRelativToParent.getX();

        //The right edge in the parent coord frame
        auto rp=m_lastBoundsRelativToParent.getTopRight().getX();

        //Get the mouse down position in the parent coordinate frame 
        auto mp=e.getMouseDownX()+lp;

        //Transform left(right) position with affine transform
        //The coordinate frame is translated to the frame with the mouse at the origin
        auto lp_trans=s*(lp-mp)+mp;
        auto rp_trans=s*(rp-mp)+mp;


        

        //new width
        auto width=rp_trans-lp_trans;
        Rectangle<int> newbounds(m_lastBoundsRelativToParent);
        newbounds.setX(lp_trans+e.getDistanceFromDragStartX());
        newbounds.setWidth(width);
        setBounds(newbounds);



        



    }
    repaint();
    //[/UserCode_mouseDrag]
}

void GraphArea::mouseDoubleClick (const juce::MouseEvent& e)
{
    //[UserCode_mouseDoubleClick] -- Add your code here...
    cout<<"Douuble click timeout: "<<e.getDoubleClickTimeout()<<endl;
    setBoundsRelative(0,0,1,1);
    //[/UserCode_mouseDoubleClick]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void GraphArea::drawGraphs(shared_ptr<GraphVector> graphs)
{
    m_graphs = graphs;
    repaint();
}

Rectangle<float> GraphArea::getGraphBounds()
{
    Rectangle<float> res;
    if (m_graphs)
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
  <METHODS>
    <METHOD name="mouseDown (const juce::MouseEvent&amp; e)"/>
    <METHOD name="mouseDrag (const juce::MouseEvent&amp; e)"/>
    <METHOD name="mouseDoubleClick (const juce::MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

