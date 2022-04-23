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

#include "MainArea.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MainArea::MainArea ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    m_controlsArea.reset (new juce::GroupComponent ("controls",
                                                    TRANS("controls")));
    addAndMakeVisible (m_controlsArea.get());
    m_controlsArea->setTextLabelPosition (juce::Justification::centred);

    m_noteClSelector.reset (new juce::ComboBox ("NoteClSelector"));
    addAndMakeVisible (m_noteClSelector.get());
    m_noteClSelector->setEditableText (false);
    m_noteClSelector->setJustificationType (juce::Justification::centredLeft);
    m_noteClSelector->setTextWhenNothingSelected (juce::String());
    m_noteClSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    m_noteClSelector->addListener (this);

    m_responseArea.reset (new ResponseArea());
    addAndMakeVisible (m_responseArea.get());
    m_responseArea->setName ("responseArea");

    juce__viewport.reset (new juce::Viewport ("new viewport"));
    addAndMakeVisible (juce__viewport.get());

    juce__viewport->setBounds (584, 40, 150, 150);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

MainArea::~MainArea()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    m_controlsArea = nullptr;
    m_noteClSelector = nullptr;
    m_responseArea = nullptr;
    juce__viewport = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MainArea::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MainArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    m_controlsArea->setBounds (0, proportionOfHeight (0.6005f), proportionOfWidth (1.0000f), proportionOfHeight (0.3995f));
    m_noteClSelector->setBounds (0 + juce::roundToInt (proportionOfWidth (1.0000f) * 0.0477f), proportionOfHeight (0.6005f) + juce::roundToInt (proportionOfHeight (0.3995f) * 0.1523f), juce::roundToInt (proportionOfWidth (1.0000f) * 0.1311f), juce::roundToInt (proportionOfHeight (0.3995f) * 0.0690f));
    m_responseArea->setBounds (proportionOfWidth (0.0596f), proportionOfHeight (0.2756f), proportionOfWidth (0.9404f), proportionOfHeight (0.3249f));
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MainArea::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == m_noteClSelector.get())
    {
        //[UserComboBoxCode_m_noteClSelector] -- add your combo box handling code here..
        //[/UserComboBoxCode_m_noteClSelector]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MainArea" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <GROUPCOMPONENT name="controls" id="5dfc032bfdecdadc" memberName="m_controlsArea"
                  virtualName="" explicitFocusOrder="0" pos="0 60.046% 100% 39.954%"
                  title="controls" textpos="36"/>
  <COMBOBOX name="NoteClSelector" id="851f113844c285f7" memberName="m_noteClSelector"
            virtualName="" explicitFocusOrder="0" pos="4.769% 15.23% 13.115% 6.897%"
            posRelativeX="5dfc032bfdecdadc" posRelativeY="5dfc032bfdecdadc"
            posRelativeW="5dfc032bfdecdadc" posRelativeH="5dfc032bfdecdadc"
            editable="0" layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="responseArea" id="93cfb025f2a0b2ff" memberName="m_responseArea"
                    virtualName="" explicitFocusOrder="0" pos="5.961% 27.555% 94.039% 32.491%"
                    class="ResponseArea" params=""/>
  <VIEWPORT name="new viewport" id="cc836f2d38e6005e" memberName="juce__viewport"
            virtualName="" explicitFocusOrder="0" pos="584 40 150 150" vscroll="1"
            hscroll="1" scrollbarThickness="8" contentType="0" jucerFile=""
            contentClass="" constructorParams=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

