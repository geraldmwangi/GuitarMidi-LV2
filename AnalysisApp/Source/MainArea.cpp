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
MainArea::MainArea()
{
    //[Constructor_pre] You can add your own custom stuff here..
    m_fretboard = make_shared<FretBoard>(nullptr, 48000);
    m_fretboard->initialize();
    m_responseArea.reset(new PlotArea());
    m_phaseArea.reset(new PlotArea());
    m_waveFileView.reset(new WaveFileView());
    //[/Constructor_pre]

    m_phaseResponseTab.reset(new juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop));
    addAndMakeVisible(m_phaseResponseTab.get());
    m_phaseResponseTab->setTabBarDepth(30);
    m_phaseResponseTab->setCurrentTabIndex(-1);

    m_controlsArea.reset(new juce::GroupComponent("controls",
                                                  TRANS("controls")));
    addAndMakeVisible(m_controlsArea.get());
    m_controlsArea->setTextLabelPosition(juce::Justification::centred);

    m_noteClSelector.reset(new juce::ComboBox("NoteClSelector"));
    addAndMakeVisible(m_noteClSelector.get());
    m_noteClSelector->setEditableText(false);
    m_noteClSelector->setJustificationType(juce::Justification::centredLeft);
    m_noteClSelector->setTextWhenNothingSelected(juce::String());
    m_noteClSelector->setTextWhenNoChoicesAvailable(TRANS("(no choices)"));
    m_noteClSelector->addListener(this);

    m_bandwidthInput.reset(new juce::Slider("bandwidthInput"));
    addAndMakeVisible(m_bandwidthInput.get());
    m_bandwidthInput->setRange(10, 50, 0);
    m_bandwidthInput->setSliderStyle(juce::Slider::Rotary);
    m_bandwidthInput->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);
    m_bandwidthInput->addListener(this);

    m_waveFileGroup.reset(new juce::GroupComponent("new group",
                                                   TRANS("group")));
    addAndMakeVisible(m_waveFileGroup.get());

    //[UserPreSize]

    m_waveFileGroup->addChildComponent(m_waveFileView.get());
    m_waveFileView->setVisible(true);
    m_waveFileView->addChangeListener(this);
    m_phaseResponseTab->addTab(TRANS("Response"), juce::Colours::lightgrey, m_responseArea.get(), false);
    m_phaseResponseTab->addTab(TRANS("Phase"), juce::Colours::lightgrey, m_phaseArea.get(), false);

    m_noteClSelector->addItem("ALL", ALL_NOTECLS);
    for (int n = 0; n < m_fretboard->getNoteClassifiers().size(); n++)
    {
        auto notecl = m_fretboard->getNoteClassifiers()[n];
        m_noteClSelector->addItem(String(notecl->getCenterFrequency()), n);
    }
    //[/UserPreSize]

    setSize(600, 400);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

MainArea::~MainArea()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    m_phaseResponseTab = nullptr;
    m_controlsArea = nullptr;
    m_noteClSelector = nullptr;
    m_bandwidthInput = nullptr;
    m_waveFileGroup = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MainArea::paint(juce::Graphics &g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll(juce::Colour(0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MainArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    m_phaseResponseTab->setBounds(proportionOfWidth(0.0179f), proportionOfHeight(0.0188f), proportionOfWidth(0.9717f), proportionOfHeight(0.5511f));
    m_controlsArea->setBounds(0, proportionOfHeight(0.6005f), proportionOfWidth(1.0000f), proportionOfHeight(0.3995f));
    m_noteClSelector->setBounds(0 + juce::roundToInt(proportionOfWidth(1.0000f) * 0.0477f), proportionOfHeight(0.6005f) + juce::roundToInt(proportionOfHeight(0.3995f) * 0.1500f), juce::roundToInt(proportionOfWidth(1.0000f) * 0.1311f), juce::roundToInt(proportionOfHeight(0.3995f) * 0.0677f));
    m_bandwidthInput->setBounds(proportionOfWidth(0.2496f), proportionOfHeight(0.6710f), proportionOfWidth(0.2049f), proportionOfHeight(0.1328f));
    m_waveFileGroup->setBounds(proportionOfWidth(0.4590f), proportionOfHeight(0.6769f), proportionOfWidth(0.5067f), proportionOfHeight(0.1763f));
    //[UserResized] Add your own custom resize handling here..
    m_waveFileView->setBounds(m_waveFileGroup->getLocalBounds());
    //[/UserResized]
}

void MainArea::comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == m_noteClSelector.get())
    {
        //[UserComboBoxCode_m_noteClSelector] -- add your combo box handling code here..
        int id = comboBoxThatHasChanged->getSelectedId();
        m_currentNoteCl = id;
        shared_ptr<GraphVector> responseGraphs = make_shared<GraphVector>();
        shared_ptr<GraphVector> phaseGraphs = make_shared<GraphVector>();
        if (m_currentNoteCl == ALL_NOTECLS)
        {

            for (auto notecl : m_fretboard->getNoteClassifiers())
            {

                shared_ptr<ResponseGraph> newspektrum = make_shared<ResponseGraph>(notecl,m_waveFileView->getCurrentAudioSlice());
                shared_ptr<PhaseGraph> newphase = make_shared<PhaseGraph>(notecl);
                responseGraphs->push_back(newspektrum);
                phaseGraphs->push_back(newphase);
            }
        }
        else
        {
            auto notecl = m_fretboard->getNoteClassifiers()[m_currentNoteCl];
            shared_ptr<ResponseGraph> newspektrum = make_shared<ResponseGraph>(notecl,m_waveFileView->getCurrentAudioSlice());
            shared_ptr<PhaseGraph> newphase = make_shared<PhaseGraph>(notecl);
            responseGraphs->push_back(newspektrum);
            phaseGraphs->push_back(newphase);
        }

        m_responseArea->drawGraphs(responseGraphs);
        m_phaseArea->drawGraphs(phaseGraphs);
        //[/UserComboBoxCode_m_noteClSelector]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void MainArea::sliderValueChanged(juce::Slider *sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == m_bandwidthInput.get())
    {
        //[UserSliderCode_m_bandwidthInput] -- add your slider handling code here..
        float val = sliderThatWasMoved->getValue();
        if (m_currentNoteCl != ALL_NOTECLS)
        {
            m_fretboard->getNoteClassifiers()[m_currentNoteCl]->setFilterParameters(val);
        }
        else
        {
            for (auto notecl : m_fretboard->getNoteClassifiers())
            {
                notecl->setFilterParameters(val);
            }
        }
        // m_responseArea->drawGraphs(m_currentNoteCl);
        // m_phaseArea->drawGraphs(m_currentNoteCl);
        repaint();
        //[/UserSliderCode_m_bandwidthInput]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void MainArea::changeListenerCallback(ChangeBroadcaster *source)
{
    if(source==dynamic_cast<ChangeBroadcaster*>(m_waveFileView.get()))
    {
        cout<<"MainArea waveFileView changed"<<endl;
    }
}
//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MainArea" componentName=""
                 parentClasses="public juce::Component, public juce::ChangeListener"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="0" initialWidth="600"
                 initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <TABBEDCOMPONENT name="new tabbed component" id="d26db19854bd82c8" memberName="m_phaseResponseTab"
                   virtualName="" explicitFocusOrder="0" pos="1.75% 1.88% 97.25% 55.112%"
                   orientation="top" tabBarDepth="30" initialTab="-1"/>
  <GROUPCOMPONENT name="controls" id="5dfc032bfdecdadc" memberName="m_controlsArea"
                  virtualName="" explicitFocusOrder="0" pos="0 60.047% 100% 39.953%"
                  title="controls" textpos="36"/>
  <COMBOBOX name="NoteClSelector" id="851f113844c285f7" memberName="m_noteClSelector"
            virtualName="" explicitFocusOrder="0" pos="4.75% 15% 13% 6.765%"
            posRelativeX="5dfc032bfdecdadc" posRelativeY="5dfc032bfdecdadc"
            posRelativeW="5dfc032bfdecdadc" posRelativeH="5dfc032bfdecdadc"
            editable="0" layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <SLIDER name="bandwidthInput" id="1373a80496253bfb" memberName="m_bandwidthInput"
          virtualName="" explicitFocusOrder="0" pos="25% 67.098% 20.5% 13.278%"
          min="10.0" max="50.0" int="0.0" style="Rotary" textBoxPos="TextBoxLeft"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <GROUPCOMPONENT name="new group" id="3dba363dad72d9cb" memberName="m_waveFileGroup"
                  virtualName="" explicitFocusOrder="0" pos="46% 67.685% 50.75% 17.626%"
                  title="group"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//[EndFile] You can add extra defines here...
//[/EndFile]
