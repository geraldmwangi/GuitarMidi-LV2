/*
  ==============================================================================

    FilterResponse.h
    Created: 18 Apr 2022 8:36:03pm
    Author:  gerald

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class FilterResponse  : public juce::Component
{
public:
    FilterResponse();
    ~FilterResponse() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterResponse)
};
