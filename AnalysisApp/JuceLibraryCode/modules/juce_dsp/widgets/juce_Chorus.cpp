/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce::dsp
{

//==============================================================================
template <typename SampleType>
Chorus<SampleType>::Chorus()
{
    auto oscFunction = [] (SampleType x) { return std::sin (x); };
    osc.initialise (oscFunction);

    dryWet.setMixingRule (DryWetMixingRule::linear);
}

template <typename SampleType>
void Chorus<SampleType>::setRate (SampleType newRateHz)
{
    jassert (isPositiveAndBelow (newRateHz, static_cast<SampleType> (100.0)));

    rate = newRateHz;
    update();
}

template <typename SampleType>
void Chorus<SampleType>::setDepth (SampleType newDepth)
{
    jassert (isPositiveAndNotGreaterThan (newDepth, maxDepth));

    depth = newDepth;
    update();
}

template <typename SampleType>
void Chorus<SampleType>::setCentreDelay (SampleType newDelayMs)
{
    jassert (isPositiveAndBelow (newDelayMs, maxCentreDelayMs));

    centreDelay = jlimit (static_cast<SampleType> (1.0), maxCentreDelayMs, newDelayMs);
}

template <typename SampleType>
void Chorus<SampleType>::setFeedback (SampleType newFeedback)
{
    jassert (newFeedback >= static_cast<SampleType> (-1.0) && newFeedback <= static_cast<SampleType> (1.0));

    feedback = newFeedback;
    update();
}

template <typename SampleType>
void Chorus<SampleType>::setMix (SampleType newMix)
{
    jassert (isPositiveAndNotGreaterThan (newMix, static_cast<SampleType> (1.0)));

    mix = newMix;
    update();
}

//==============================================================================
template <typename SampleType>
void Chorus<SampleType>::prepare (const ProcessSpec& spec)
{
    jassert (spec.sampleRate > 0);
    jassert (spec.numChannels > 0);

    sampleRate = spec.sampleRate;

    const auto maxPossibleDelay = std::ceil ((maximumDelayModulation * maxDepth * oscVolumeMultiplier + maxCentreDelayMs)
                                             * sampleRate / 1000.0);
    delay = DelayLine<SampleType, DelayLineInterpolationTypes::Linear>{ static_cast<int> (maxPossibleDelay) };
    delay.prepare (spec);

    dryWet.prepare (spec);
    feedbackVolume.resize (spec.numChannels);
    lastOutput.resize (spec.numChannels);

    osc.prepare (spec);
    bufferDelayTimes.setSize (1, (int) spec.maximumBlockSize, false, false, true);

    update();
    reset();
}

template <typename SampleType>
void Chorus<SampleType>::reset()
{
    std::fill (lastOutput.begin(), lastOutput.end(), static_cast<SampleType> (0));

    delay.reset();
    osc.reset();
    dryWet.reset();

    oscVolume.reset (sampleRate, 0.05);

    for (auto& vol : feedbackVolume)
        vol.reset (sampleRate, 0.05);
}

template <typename SampleType>
void Chorus<SampleType>::update()
{
    osc.setFrequency (rate);
    oscVolume.setTargetValue (depth * oscVolumeMultiplier);
    dryWet.setWetMixProportion (mix);

    for (auto& vol : feedbackVolume)
        vol.setTargetValue (feedback);
}

//==============================================================================
template class Chorus<float>;
template class Chorus<double>;

} // namespace juce::dsp
