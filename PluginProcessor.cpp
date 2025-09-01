#include "PluginProcessor.h"
#include "PluginEditor.h"

PannerAudioProcessor::PannerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, juce::Identifier("params"),
        {
            std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID("PAN", 1), "Pan",
                juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f),
            std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID("PAN_LAW", 2), "Pan Law",
                0, 3, 1)
        }),
    smoothedLeftGain(0.0f),
    smoothedRightGain(0.0f)
{
}

PannerAudioProcessor::~PannerAudioProcessor() {}

void PannerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize smoothers with a ramp time (e.g., 50 ms) to avoid clicks
    smoothedLeftGain.reset(sampleRate, 0.05f);
    smoothedRightGain.reset(sampleRate, 0.05f);
    smoothedLeftGain.setCurrentAndTargetValue(0.5f);  // Center pan
    smoothedRightGain.setCurrentAndTargetValue(0.5f); // Center pan
}

void PannerAudioProcessor::releaseResources() {}

void PannerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numSamples = buffer.getNumSamples();
    const int numChans = buffer.getNumChannels();
    if (numChans < 2) return;

    float pan = *parameters.getRawParameterValue("PAN");
    int panLaw = (int)*parameters.getRawParameterValue("PAN_LAW");

    float targetLeftGain = 0.0f;
    float targetRightGain = 0.0f;

    switch (panLaw)
    {
    case 0: // Linear
        targetLeftGain = (1.0f - pan) * 0.5f;
        targetRightGain = (1.0f + pan) * 0.5f;
        break;
    case 1: // -3 dB Constant Power
        targetLeftGain = std::cos((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        targetRightGain = std::sin((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        break;
    case 2: // Square-root
        targetLeftGain = std::sqrt(1.0f - ((pan + 1.0f) * 0.5f));
        targetRightGain = std::sqrt(((pan + 1.0f) * 0.5f));
        break;
    case 3: // Balance
        targetLeftGain = pan <= 0.0f ? 1.0f : 1.0f - pan;
        targetRightGain = pan >= 0.0f ? 1.0f : 1.0f + pan;
        break;
    }

    // Set target values for smoothers
    smoothedLeftGain.setTargetValue(targetLeftGain);
    smoothedRightGain.setTargetValue(targetRightGain);

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    // Apply smoothed gains sample-by-sample
    for (int i = 0; i < numSamples; ++i)
    {
        float inL = left[i];
        float inR = right[i];

        float currentLeftGain = smoothedLeftGain.getNextValue();
        float currentRightGain = smoothedRightGain.getNextValue();

        if (panLaw == 3) // Balance mode: no cross-mixing
        {
            left[i] = inL * currentLeftGain;
            right[i] = inR * currentRightGain;
        }
        else // Other modes: stricter panning with minimal cross-mixing
        {
            left[i] = inL * currentLeftGain;
            right[i] = inR * currentRightGain;
        }
    }
}

juce::AudioProcessorEditor* PannerAudioProcessor::createEditor() { return new PannerAudioProcessorEditor(*this); }
bool PannerAudioProcessor::hasEditor() const { return true; }

const juce::String PannerAudioProcessor::getName() const { return "SuperPan"; }
bool PannerAudioProcessor::acceptsMidi() const { return false; }
bool PannerAudioProcessor::producesMidi() const { return false; }
double PannerAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int PannerAudioProcessor::getNumPrograms() { return 1; }
int PannerAudioProcessor::getCurrentProgram() { return 0; }
void PannerAudioProcessor::setCurrentProgram(int) {}
const juce::String PannerAudioProcessor::getProgramName(int) { return {}; }
void PannerAudioProcessor::changeProgramName(int, const juce::String&) {}
void PannerAudioProcessor::getStateInformation(juce::MemoryBlock& /*destData*/) {}
void PannerAudioProcessor::setStateInformation(const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PannerAudioProcessor();
}