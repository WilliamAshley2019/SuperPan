#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PannerAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    PannerAudioProcessorEditor(PannerAudioProcessor&);
    ~PannerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PannerAudioProcessor& audioProcessor;

    juce::Slider panSlider;
    juce::ComboBox panLawBox;
    juce::Rectangle<float> curveArea;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> panLawAttachment;

    float currentLeftGain = 0.5f;
    float currentRightGain = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PannerAudioProcessorEditor)
};