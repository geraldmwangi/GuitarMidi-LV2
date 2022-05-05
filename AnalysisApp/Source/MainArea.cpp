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
    m_orderInput->setRange (1, 10, 1);
    m_orderInput->setSliderStyle (juce::Slider::IncDecButtons);
    m_orderInput->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    m_orderInput->addListener (this);

    m_onsetMethod.reset (new juce::ComboBox ("onsetMethod"));
    addAndMakeVisible (m_onsetMethod.get());
    m_onsetMethod->setEditableText (false);
    m_onsetMethod->setJustificationType (juce::Justification::centredLeft);
    m_onsetMethod->setTextWhenNothingSelected (TRANS("select onset method"));
    m_onsetMethod->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    m_onsetMethod->addItem (TRANS("energy"), 1);
    m_onsetMethod->addItem (TRANS("hfc"), 2);
    m_onsetMethod->addItem (TRANS("complex"), 3);
    m_onsetMethod->addItem (TRANS("phase"), 4);
    m_onsetMethod->addItem (TRANS("wphase"), 5);
    m_onsetMethod->addItem (TRANS("specdiff"), 6);
    m_onsetMethod->addItem (TRANS("kl"), 7);
    m_onsetMethod->addItem (TRANS("mkl"), 8);
    m_onsetMethod->addItem (TRANS("specflux"), 9);
    m_onsetMethod->addListener (this);

    m_onsetthreshold.reset (new juce::Slider ("onsetthreshold"));
    addAndMakeVisible (m_onsetthreshold.get());
    m_onsetthreshold->setTooltip (TRANS("onset threshold"));
    m_onsetthreshold->setRange (0.1, 0.9, 0);
    m_onsetthreshold->setSliderStyle (juce::Slider::Rotary);
    m_onsetthreshold->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    m_onsetthreshold->addListener (this);

    m_onsetsilence.reset (new juce::Slider ("onsetsilence"));
    addAndMakeVisible (m_onsetsilence.get());
    m_onsetsilence->setTooltip (TRANS("onset threshold"));
    m_onsetsilence->setRange (-90, -10, 0);
    m_onsetsilence->setSliderStyle (juce::Slider::Rotary);
    m_onsetsilence->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    m_onsetsilence->addListener (this);

    m_onsetcomp.reset (new juce::Slider ("onsetcomp"));
    addAndMakeVisible (m_onsetcomp.get());
    m_onsetcomp->setTooltip (TRANS("onset compression"));
    m_onsetcomp->setRange (0, 1, 0);
    m_onsetcomp->setSliderStyle (juce::Slider::Rotary);
    m_onsetcomp->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    m_onsetcomp->addListener (this);


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
    m_noteClSelector->setSelectedId(ALL_NOTECLS);
    m_onsetMethod->setSelectedId(1);
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
    m_onsetMethod = nullptr;
    m_onsetthreshold = nullptr;
    m_onsetsilence = nullptr;
    m_onsetcomp = nullptr;


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
        int x = proportionOfWidth (0.4441f), y = proportionOfHeight (0.8320f), width = proportionOfWidth (0.1490f), height = proportionOfHeight (0.0352f);
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
        int x = proportionOfWidth (0.5753f), y = proportionOfHeight (0.8320f), width = proportionOfWidth (0.1490f), height = proportionOfHeight (0.0352f);
        juce::String text (TRANS("Filter order"));
        juce::Colour fillColour = juce::Colours::white;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
        g.drawText (text, x, y, width, height,
                    juce::Justification::centred, true);
    }

    {
        int x = proportionOfWidth (0.0268f), y = proportionOfHeight (0.8320f), width = proportionOfWidth (0.0805f), height = proportionOfHeight (0.0352f);
        juce::String text (TRANS("Peak Threshold"));
        juce::Colour fillColour = juce::Colour (0xfffefefe);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
        g.drawText (text, x, y, width, height,
                    juce::Justification::centred, true);
    }

    {
        int x = proportionOfWidth (0.1461f), y = proportionOfHeight (0.8320f), width = proportionOfWidth (0.0745f), height = proportionOfHeight (0.0352f);
        juce::String text (TRANS("Onset silence"));
        juce::Colour fillColour = juce::Colours::white;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
        g.drawText (text, x, y, width, height,
                    juce::Justification::centred, true);
    }

    {
        int x = proportionOfWidth (0.2474f), y = proportionOfHeight (0.8320f), width = proportionOfWidth (0.1043f), height = proportionOfHeight (0.0352f);
        juce::String text (TRANS("Onset compression"));
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

    m_phaseResponseTab->setBounds (proportionOfWidth (0.0171f), proportionOfHeight (0.0188f), proportionOfWidth (0.9724f), proportionOfHeight (0.5511f));
    m_controlsArea->setBounds (0, proportionOfHeight (0.6005f), proportionOfWidth (1.0000f), proportionOfHeight (0.3995f));
    m_noteClSelector->setBounds (0 + juce::roundToInt (proportionOfWidth (1.0000f) * 0.0477f), proportionOfHeight (0.6005f) + juce::roundToInt (proportionOfHeight (0.3995f) * 0.1500f), juce::roundToInt (proportionOfWidth (1.0000f) * 0.1297f), juce::roundToInt (proportionOfHeight (0.3995f) * 0.0677f));
    m_bandwidthInput->setBounds (proportionOfWidth (0.4471f), proportionOfHeight (0.8649f), proportionOfWidth (0.1513f), proportionOfHeight (0.1328f));
    m_waveFileGroup->setBounds (proportionOfWidth (0.2265f), proportionOfHeight (0.6299f), proportionOfWidth (0.7511f), proportionOfHeight (0.1763f));
    m_orderInput->setBounds (proportionOfWidth (0.5961f), proportionOfHeight (0.8837f), proportionOfWidth (0.1155f), proportionOfHeight (0.0752f));
    m_onsetMethod->setBounds (proportionOfWidth (0.0477f), proportionOfHeight (0.7239f), proportionOfWidth (0.1252f), proportionOfHeight (0.0282f));
    m_onsetthreshold->setBounds (proportionOfWidth (0.0179f), proportionOfHeight (0.8649f), proportionOfWidth (0.0954f), proportionOfHeight (0.1222f));
    m_onsetsilence->setBounds (proportionOfWidth (0.1371f), proportionOfHeight (0.8649f), proportionOfWidth (0.0954f), proportionOfHeight (0.1222f));
    m_onsetcomp->setBounds (proportionOfWidth (0.2444f), proportionOfHeight (0.8649f), proportionOfWidth (0.0954f), proportionOfHeight (0.1222f));
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
    else if (comboBoxThatHasChanged == m_onsetMethod.get())
    {
        //[UserComboBoxCode_m_onsetMethod] -- add your combo box handling code here..
        setOnsetDetectors();
        //[/UserComboBoxCode_m_onsetMethod]
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
    else if (sliderThatWasMoved == m_onsetthreshold.get())
    {
        //[UserSliderCode_m_onsetthreshold] -- add your slider handling code here..
        setOnsetDetectors();
        //[/UserSliderCode_m_onsetthreshold]
    }
    else if (sliderThatWasMoved == m_onsetsilence.get())
    {
        //[UserSliderCode_m_onsetsilence] -- add your slider handling code here..
        setOnsetDetectors();
        //[/UserSliderCode_m_onsetsilence]
    }
    else if (sliderThatWasMoved == m_onsetcomp.get())
    {
        //[UserSliderCode_m_onsetcomp] -- add your slider handling code here..
        setOnsetDetectors();
        //[/UserSliderCode_m_onsetcomp]
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
void MainArea::setOnsetDetectors()
{
        auto id=m_onsetMethod->getSelectedItemIndex();
        auto val=m_onsetMethod->getItemText(id);
        auto threshold=m_onsetthreshold->getValue();
        auto silence=m_onsetsilence->getValue();
        auto comp=m_onsetcomp->getValue();
        for(auto notecl:m_fretboard->getNoteClassifiers())
        {
            notecl->setOnsetParameter(val.toStdString(),threshold,silence,comp);
        }
        drawGraphs();
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
    <TEXT pos="44.411% 83.196% 14.903% 3.525%" fill="solid: ffffffff" hasStroke="0"
          text="Bandwidth" fontname="Default font" fontsize="15.0" kerning="0.0"
          bold="0" italic="0" justification="36"/>
    <TEXT pos="57.526% 83.196% 14.903% 3.525%" fill="solid: ffffffff" hasStroke="0"
          text="Filter order" fontname="Default font" fontsize="15.0" kerning="0.0"
          bold="0" italic="0" justification="36"/>
    <TEXT pos="2.683% 83.196% 8.048% 3.525%" fill="solid: fffefefe" hasStroke="0"
          text="Peak Threshold" fontname="Default font" fontsize="15.0"
          kerning="0.0" bold="0" italic="0" justification="36"/>
    <TEXT pos="14.605% 83.196% 7.452% 3.525%" fill="solid: ffffffff" hasStroke="0"
          text="Onset silence" fontname="Default font" fontsize="15.0"
          kerning="0.0" bold="0" italic="0" justification="36"/>
    <TEXT pos="24.739% 83.196% 10.432% 3.525%" fill="solid: ffffffff" hasStroke="0"
          text="Onset compression" fontname="Default font" fontsize="15.0"
          kerning="0.0" bold="0" italic="0" justification="36"/>
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
          virtualName="" explicitFocusOrder="0" pos="44.709% 86.486% 15.127% 13.278%"
          tooltip="Bandwidth" min="10.0" max="50.0" int="0.0" style="Rotary"
          textBoxPos="TextBoxBelow" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <GROUPCOMPONENT name="new group" id="3dba363dad72d9cb" memberName="m_waveFileGroup"
                  virtualName="" explicitFocusOrder="0" pos="22.653% 62.985% 75.112% 17.626%"
                  title="Input Wavefile"/>
  <SLIDER name="orderInput" id="9776d67d8e8f0565" memberName="m_orderInput"
          virtualName="" explicitFocusOrder="0" pos="59.613% 88.367% 11.55% 7.521%"
          tooltip="Filter order" min="1.0" max="10.0" int="1.0" style="IncDecButtons"
          textBoxPos="TextBoxBelow" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <COMBOBOX name="onsetMethod" id="e193ce76d9fca56d" memberName="m_onsetMethod"
            virtualName="" explicitFocusOrder="0" pos="4.769% 72.385% 12.519% 2.82%"
            editable="0" layout="33" items="energy&#10;hfc&#10;complex&#10;phase&#10;wphase&#10;specdiff&#10;kl&#10;mkl&#10;specflux"
            textWhenNonSelected="select onset method" textWhenNoItems="(no choices)"/>
  <SLIDER name="onsetthreshold" id="a007d5251242647" memberName="m_onsetthreshold"
          virtualName="" explicitFocusOrder="0" pos="1.788% 86.486% 9.538% 12.221%"
          tooltip="onset threshold" min="0.1" max="0.9" int="0.0" style="Rotary"
          textBoxPos="TextBoxBelow" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <SLIDER name="onsetsilence" id="beff56a9655692c4" memberName="m_onsetsilence"
          virtualName="" explicitFocusOrder="0" pos="13.711% 86.486% 9.538% 12.221%"
          tooltip="onset threshold" min="-90.0" max="-10.0" int="0.0" style="Rotary"
          textBoxPos="TextBoxBelow" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <SLIDER name="onsetcomp" id="a06d44331028ba76" memberName="m_onsetcomp"
          virtualName="" explicitFocusOrder="0" pos="24.441% 86.486% 9.538% 12.221%"
          tooltip="onset compression" min="0.0" max="1.0" int="0.0" style="Rotary"
          textBoxPos="TextBoxBelow" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

