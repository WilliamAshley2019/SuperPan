#include "PluginProcessor.h"
#include "PluginEditor.h"

PannerAudioProcessorEditor::PannerAudioProcessorEditor(PannerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 250);

    // Pan Slider
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    panSlider.setRange(-1.0, 1.0, 0.01);
    addAndMakeVisible(panSlider);

    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "PAN", panSlider);

    // Pan Law Selector
    panLawBox.addItem("Linear", 0 + 1);
    panLawBox.addItem("-3 dB Constant Power", 1 + 1);
    panLawBox.addItem("Square-root", 2 + 1);
    panLawBox.addItem("Balance", 3 + 1);
    addAndMakeVisible(panLawBox);

    panLawAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.parameters, "PAN_LAW", panLawBox);

    // CPU-efficient refresh rate (30 Hz is plenty for visual feedback)
    startTimerHz(30);
}

PannerAudioProcessorEditor::~PannerAudioProcessorEditor() {}

void PannerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    // Draw stereo meters with smooth animation
    g.setColour(juce::Colours::lime);
    float leftMeterHeight = currentLeftGain * 100.0f;
    g.fillRect(50.0f, 50.0f + (100.0f - leftMeterHeight), 50.0f, leftMeterHeight);

    g.setColour(juce::Colours::cyan);
    float rightMeterHeight = currentRightGain * 100.0f;
    g.fillRect(150.0f, 50.0f + (100.0f - rightMeterHeight), 50.0f, rightMeterHeight);

    // Draw meter outlines
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(50.0f, 50.0f, 50.0f, 100.0f, 1.0f);
    g.drawRect(150.0f, 50.0f, 50.0f, 100.0f, 1.0f);

    // Draw labels
    g.setColour(juce::Colours::white);
    g.drawText("L", 50, 160, 50, 20, juce::Justification::centred);
    g.drawText("R", 150, 160, 50, 20, juce::Justification::centred);

    // Draw gain values
    g.setFont(10.0f);
    g.drawText(juce::String(currentLeftGain, 2), 50, 35, 50, 15, juce::Justification::centred);
    g.drawText(juce::String(currentRightGain, 2), 150, 35, 50, 15, juce::Justification::centred);

    // Draw pan law curve
    g.setColour(juce::Colours::yellow);
    juce::Path leftPath, rightPath;

    float pan = *audioProcessor.parameters.getRawParameterValue("PAN");
    int panLaw = (int)*audioProcessor.parameters.getRawParameterValue("PAN_LAW");

    const int steps = 100;
    float w = curveArea.getWidth();
    float h = curveArea.getHeight();

    for (int i = 0; i <= steps; ++i)
    {
        float p = -1.0f + (2.0f * i / steps);
        float leftGain = 0.0f, rightGain = 0.0f;

        switch (panLaw)
        {
        case 0: // Linear
            leftGain = (1.0f - p) * 0.5f;
            rightGain = (1.0f + p) * 0.5f;
            break;
        case 1: // -3 dB Constant Power
            leftGain = std::cos((p + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            rightGain = std::sin((p + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            break;
        case 2: // Square-root
            leftGain = std::sqrt(1.0f - ((p + 1.0f) * 0.5f));
            rightGain = std::sqrt(((p + 1.0f) * 0.5f));
            break;
        case 3: // Balance
            leftGain = p <= 0.0f ? 1.0f : 1.0f - p;
            rightGain = p >= 0.0f ? 1.0f : 1.0f + p;
            break;
        }

        float x = curveArea.getX() + (i * w / steps);
        float yLeft = curveArea.getY() + h * (1.0f - leftGain);
        float yRight = curveArea.getY() + h * (1.0f - rightGain);

        if (i == 0)
        {
            leftPath.startNewSubPath(x, yLeft);
            rightPath.startNewSubPath(x, yRight);
        }
        else
        {
            leftPath.lineTo(x, yLeft);
            rightPath.lineTo(x, yRight);
        }
    }

    g.setColour(juce::Colours::lime);
    g.strokePath(leftPath, juce::PathStrokeType(1.5f));
    g.setColour(juce::Colours::cyan);
    g.strokePath(rightPath, juce::PathStrokeType(1.5f));

    // Draw current pan position indicator
    g.setColour(juce::Colours::red);
    float panX = curveArea.getX() + (pan + 1.0f) * 0.5f * w;
    g.drawLine(panX, curveArea.getY(), panX, curveArea.getBottom(), 2.0f);

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("SuperPan", 0, 5, getWidth(), 20, juce::Justification::centred);
}

void PannerAudioProcessorEditor::resized()
{
    curveArea.setBounds(250, 20, 120, 30);
    panSlider.setBounds(250, 50, 120, 120);
    panLawBox.setBounds(250, 180, 120, 30);
}

void PannerAudioProcessorEditor::timerCallback()
{
    // Get current gains from processor (thread-safe)
    currentLeftGain = audioProcessor.getCurrentLeftGain();
    currentRightGain = audioProcessor.getCurrentRightGain();

    repaint();
}