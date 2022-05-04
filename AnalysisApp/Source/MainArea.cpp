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
    m_fretboard = make_shared<FretBoard>(nullptr, 48000);
    m_fretboard->initialize();
    m_responseArea.reset(new PlotArea());
    m_meanResponseAudioArea.reset(new PlotArea());
    m_responseAudioArea.reset(new PlotArea());
    m_phaseArea.reset(new PlotArea());
    m_waveFileView.reset(new WaveFileView());
    //[/Constructor_pre]

    m_phaseResponseTab.reset (new juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop));
    addAndMakeVisible (m_phaseResponseTab.get());
    m_phaseResponseTab->setTabBarDepth (30);
    m_phaseResponseTab->setCurrentTabIndex (-1);

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

    m_bandwidthInput.reset (new juce::Slider ("bandwidthInput"));
    addAndMakeVisible (m_bandwidthInput.get());
    m_bandwidthInput->setTooltip (TRANS("Bandwidth"));
    m_bandwidthInput->setRange (10, 50, 0);
    m_bandwidthInput->setSliderStyle (juce::Slider::Rotary);
    m_bandwidthInput->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    m_bandwidthInput->addListener (this);

    m_waveFileGroup.reset (new juce::GroupComponent ("new group",
                                                     TRANS("Input Wavefile")));
    addAndMakeVisible (m_waveFileGroup.get());

    m_orderInput.reset (new juce::Slider ("orderInput"));
    addAndMakeVisible (m_orderInput.get());
    m_orderInput->setTooltip (TRANS("Filter order"));
    m_orderInput->setRange (1, 10, 0);
    m_orderInput->setSliderStyle (juce::Slider::Rotary);
    m_orderInput->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    m_orderInput->addListener (this);


    //[UserPreSize]

    m_waveFileGroup->addChildComponent(m_waveFileView.get());
    m_waveFileView->setVisible(true);
    m_waveFileView->addChangeListener(this);
    m_phaseResponseTab->addTab(TRANS("Response"), juce::Colours::lightgrey, m_responseArea.get(), false);
    m_phaseResponseTab->addTab(TRANS("Phase"), juce::Colours::lightgrey, m_phaseArea.get(), false);
    m_phaseResponseTab->addTab(TRANS("Mean Response Audio"), juce::Colours::lightgrey, m_meanResponseAudioArea.get(), false);
    m_phaseResponseTab->addTab(TRANS(" Response Audio"), juce::Colours::lightgrey, m_responseAudioArea.get(), false);

    m_noteClSelector->addItem("ALL", ALL_NOTECLS);
    for (int n = 0; n < m_fretboard->getNoteClassifiers().size(); n++)
    {
        auto notecl = m_fretboard->getNoteClassifiers()[n];
        m_noteClSelector->addItem(String(notecl->getCenterFrequency()), n+1);//IDs in the combobox cannot be zero thats why all IDs are shifted by 1
    }
    //[/UserPreSize]

    setSize (600, 400);


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
    m_orderInput = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MainArea::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    {
        int x = proportionOfWidth (0.2295f), y = proportionOfHeight (0.6345f), width = proportionOfWidth (0.1490f), height = proportionOfHeight (0.0353f);
        juce::String text (TRANS("Bandwidth"));
        juce::Colour fillColour = juce::Colours::white;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
        g.drawText (text, x, y, width, height,
                    juce::Justification::centred, true);
    }

    {
        int x = proportionOfWidth (0.2295f), y = proportionOfHeight (0.8226f), width = proportionOfWidth (0.1490f), height = proportionOfHeight (0.0353f);
        juce::String text (TRANS("Filter order"));
        juce::Colour fillColour = juce::Colours::white;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
        g.drawText (text, x, y, width, height,
                    juce::Justification::centred, true);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MainArea::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    m_phaseResponseTab->setBounds (proportionOfWidth (0.0175f), proportionOfHeight (0.0188f), proportionOfWidth (0.9725f), proportionOfHeight (0.5511f));
    m_controlsArea->setBounds (0, proportionOfHeight (0.6005f), proportionOfWidth (1.0000f), proportionOfHeight (0.3995f));
    m_noteClSelector->setBounds (0 + juce::roundToInt (proportionOfWidth (1.0000f) * 0.0475f), proportionOfHeight (0.6005f) + juce::roundToInt (proportionOfHeight (0.3995f) * 0.1500f), juce::roundToInt (proportionOfWidth (1.0000f) * 0.1300f), juce::roundToInt (proportionOfHeight (0.3995f) * 0.0677f));
    m_bandwidthInput->setBounds (proportionOfWidth (0.2027f), proportionOfHeight (0.6675f), proportionOfWidth (0.2049f), proportionOfHeight (0.1328f));
    m_waveFileGroup->setBounds (proportionOfWidth (0.4709f), proportionOfHeight (0.6299f), proportionOfWidth (0.5074f), proportionOfHeight (0.1763f));
    m_orderInput->setBounds (proportionOfWidth (0.2027f), proportionOfHeight (0.8555f), proportionOfWidth (0.2049f), proportionOfHeight (0.1328f));
    //[UserResized] Add your own custom resize handling here..
    m_waveFileView->setBounds(m_waveFileGroup->getLocalBounds());
    //[/UserResized]
}

void MainArea::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == m_noteClSelector.get())
    {
        //[UserComboBoxCode_m_noteClSelector] -- add your combo box handling code here..
        int id = comboBoxThatHasChanged->getSelectedId();
        m_currentNoteCl = id;
        drawGraphs();
        //[/UserComboBoxCode_m_noteClSelector]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void MainArea::sliderValueChanged (juce::Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == m_bandwidthInput.get())
    {
        //[UserSliderCode_m_bandwidthInput] -- add your slider handling code here..
        float val = sliderThatWasMoved->getValue();
        float order=m_orderInput->getValue();
        if (m_currentNoteCl != ALL_NOTECLS)
        {
            m_fretboard->getNoteClassifiers()[m_currentNoteCl-1]->setFilterParameters(val,1.0,order);
        }
        else
        {
            for (auto notecl : m_fretboard->getNoteClassifiers())
            {
                notecl->setFilterParameters(val,1.0,order);
            }
        }
        // m_responseArea->drawGraphs(m_currentNoteCl);
        // m_phaseArea->drawGraphs(m_currentNoteCl);
        drawGraphs();
        //[/UserSliderCode_m_bandwidthInput]
    }
    else if (sliderThatWasMoved == m_orderInput.get())
    {
        //[UserSliderCode_m_orderInput] -- add your slider handling code here..
        float bw = m_bandwidthInput->getValue();
        float order=m_orderInput->getValue();
        if (m_currentNoteCl != ALL_NOTECLS)
        {
            m_fretboard->getNoteClassifiers()[m_currentNoteCl-1]->setFilterParameters(bw,1.0,order);
        }
        else
        {
            for (auto notecl : m_fretboard->getNoteClassifiers())
            {
                notecl->setFilterParameters(bw,1.0,order);
            }
        }
        // m_responseArea->drawGraphs(m_currentNoteCl);
        // m_phaseArea->drawGraphs(m_currentNoteCl);
        drawGraphs();
        //[/UserSliderCode_m_orderInput]
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
        drawGraphs();
    }
}

void MainArea::drawGraphs()
{
    shared_ptr<GraphVector> responseGraphs = make_shared<GraphVector>();
    shared_ptr<GraphVector> phaseGraphs = make_shared<GraphVector>();
    shared_ptr<GraphVector> filteredAudioGraphs = make_shared<GraphVector>();
    if (m_currentNoteCl == ALL_NOTECLS)
    {

        for (auto notecl : m_fretboard->getNoteClassifiers())
        {

            shared_ptr<ResponseGraph> newspektrum = make_shared<ResponseGraph>(notecl);
            shared_ptr<MeanResponseGraph> filteredaudio = make_shared<MeanResponseGraph>(notecl, m_waveFileView->getCurrentAudioSlice());
            shared_ptr<PhaseGraph> newphase = make_shared<PhaseGraph>(notecl);
            responseGraphs->push_back(newspektrum);
            filteredAudioGraphs->push_back(filteredaudio);
            phaseGraphs->push_back(newphase);
        }
    }
    else
    {
        auto notecl = m_fretboard->getNoteClassifiers()[m_currentNoteCl-1];//IDs in the combobox cannot be zero thats why all IDs are shifted by 1
        shared_ptr<ResponseGraph> newspektrum = make_shared<ResponseGraph>(notecl);
        shared_ptr<MeanResponseGraph> filteredaudio = make_shared<MeanResponseGraph>(notecl, m_waveFileView->getCurrentAudioSlice());
        shared_ptr<PhaseGraph> newphase = make_shared<PhaseGraph>(notecl);
        responseGraphs->push_back(newspektrum);
        filteredAudioGraphs->push_back(filteredaudio);
        phaseGraphs->push_back(newphase);

        shared_ptr<GraphVector> responseAudioGraphs = make_shared<GraphVector>();

        responseAudioGraphs->push_back(make_shared<AudioResponseGraph>(notecl,m_waveFileView->getAudioBuffer()));
        m_responseAudioArea->drawGraphs(responseAudioGraphs);

    }

    m_responseArea->drawGraphs(responseGraphs);
    m_phaseArea->drawGraphs(phaseGraphs);
    m_meanResponseAudioArea->drawGraphs(filteredAudioGraphs);
    repaint();
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
  <BACKGROUND backgroundColour="ff323e44">
    <TEXT pos="22.951% 63.455% 14.903% 3.525%" fill="solid: ffffffff" hasStroke="0"
          text="Bandwidth" fontname="Default font" fontsize="15.0" kerning="0.0"
          bold="0" italic="0" justification="36"/>
    <TEXT pos="22.951% 82.256% 14.903% 3.525%" fill="solid: ffffffff" hasStroke="0"
          text="Filter order" fontname="Default font" fontsize="15.0" kerning="0.0"
          bold="0" italic="0" justification="36"/>
  </BACKGROUND>
  <TABBEDCOMPONENT name="new tabbed component" id="d26db19854bd82c8" memberName="m_phaseResponseTab"
                   virtualName="" explicitFocusOrder="0" pos="1.714% 1.88% 97.243% 55.112%"
                   orientation="top" tabBarDepth="30" initialTab="-1"/>
  <GROUPCOMPONENT name="controls" id="5dfc032bfdecdadc" memberName="m_controlsArea"
                  virtualName="" explicitFocusOrder="0" pos="0 60.047% 100% 39.953%"
                  title="controls" textpos="36"/>
  <COMBOBOX name="NoteClSelector" id="851f113844c285f7" memberName="m_noteClSelector"
            virtualName="" explicitFocusOrder="0" pos="4.769% 15% 12.966% 6.765%"
            posRelativeX="5dfc032bfdecdadc" posRelativeY="5dfc032bfdecdadc"
            posRelativeW="5dfc032bfdecdadc" posRelativeH="5dfc032bfdecdadc"
            editable="0" layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <SLIDER name="bandwidthInput" id="1373a80496253bfb" memberName="m_bandwidthInput"
          virtualName="" explicitFocusOrder="0" pos="20.268% 66.745% 20.492% 13.278%"
          tooltip="Bandwidth" min="10.0" max="50.0" int="0.0" style="Rotary"
          textBoxPos="TextBoxBelow" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <GROUPCOMPONENT name="new group" id="3dba363dad72d9cb" memberName="m_waveFileGroup"
                  virtualName="" explicitFocusOrder="0" pos="47.094% 62.985% 50.745% 17.626%"
                  title="Input Wavefile"/>
  <SLIDER name="orderInput" id="9776d67d8e8f0565" memberName="m_orderInput"
          virtualName="" explicitFocusOrder="0" pos="20.268% 85.546% 20.492% 13.278%"
          tooltip="Filter order" min="1.0" max="10.0" int="0.0" style="Rotary"
          textBoxPos="TextBoxBelow" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

