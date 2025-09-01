#include "PluginProcessor.h"
#include "PluginEditor.h"

PannerAudioProcessorEditor::PannerAudioProcessorEditor(PannerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 250);

    // Pan Slider
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
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

    startTimerHz(30); // refresh for meters and curve
}

PannerAudioProcessorEditor::~PannerAudioProcessorEditor() {}

void PannerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    // Draw stereo meters
    g.setColour(juce::Colours::lime);
    g.fillRect(50.0f, 50.0f + (1.0f - currentLeftGain) * 100.0f, 50.0f, currentLeftGain * 100.0f);
    g.setColour(juce::Colours::cyan);
    g.fillRect(150.0f, 50.0f + (1.0f - currentRightGain) * 100.0f, 50.0f, currentRightGain * 100.0f);

    // Draw labels
    g.setColour(juce::Colours::white);
    g.drawText("L", 50, 160, 50, 20, juce::Justification::centred);
    g.drawText("R", 150, 160, 50, 20, juce::Justification::centred);

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
        float p = -1.0f + (2.0f * i / steps); // Pan from -1 to 1
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
    g.strokePath(leftPath, juce::PathStrokeType(1.0f));
    g.setColour(juce::Colours::cyan);
    g.strokePath(rightPath, juce::PathStrokeType(1.0f));

    g.setColour(juce::Colours::red);
    float panX = curveArea.getX() + (pan + 1.0f) * 0.5f * w;
    g.drawLine(panX, curveArea.getY(), panX, curveArea.getBottom(), 1.0f);
}

void PannerAudioProcessorEditor::resized()
{
    curveArea.setBounds(250, 20, 120, 30);
    panSlider.setBounds(250, 50, 120, 120);
    panLawBox.setBounds(250, 180, 120, 30);
}

void PannerAudioProcessorEditor::timerCallback()
{
    float pan = *audioProcessor.parameters.getRawParameterValue("PAN");
    int panLaw = (int)*audioProcessor.parameters.getRawParameterValue("PAN_LAW");

    switch (panLaw)
    {
    case 0: // Linear
        currentLeftGain = (1.0f - pan) * 0.5f;
        currentRightGain = (1.0f + pan) * 0.5f;
        break;
    case 1: // -3 dB Constant Power
        currentLeftGain = std::cos((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        currentRightGain = std::sin((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        break;
    case 2: // Square-root
        currentLeftGain = std::sqrt(1.0f - ((pan + 1.0f) * 0.5f));
        currentRightGain = std::sqrt(((pan + 1.0f) * 0.5f));
        break;
    case 3: // Balance
        currentLeftGain = pan <= 0.0f ? 1.0f : 1.0f - pan;
        currentRightGain = pan >= 0.0f ? 1.0f : 1.0f + pan;
        break;
    }

    repaint();
}