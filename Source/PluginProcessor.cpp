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
    updateProcessFunction();
}

PannerAudioProcessor::~PannerAudioProcessor() {}

void PannerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Initialize smoothers with 20ms ramp time
    smoothedLeftGain.reset(sampleRate, 0.02);
    smoothedRightGain.reset(sampleRate, 0.02);
    smoothedLeftGain.setCurrentAndTargetValue(0.5f);
    smoothedRightGain.setCurrentAndTargetValue(0.5f);

    updateProcessFunction();
    silentBlockCount = 0;
}

void PannerAudioProcessor::releaseResources() {}

void PannerAudioProcessor::updateProcessFunction()
{
    int panLaw = (int)*parameters.getRawParameterValue("PAN_LAW");

    if (panLaw == lastPanLaw)
        return;

    lastPanLaw = panLaw;

    // Function pointer dispatch eliminates branching in hot path
    switch (panLaw)
    {
    case 0: currentProcessFunction = &PannerAudioProcessor::processLinear; break;
    case 1: currentProcessFunction = &PannerAudioProcessor::processConstantPower; break;
    case 2: currentProcessFunction = &PannerAudioProcessor::processSquareRoot; break;
    case 3: currentProcessFunction = &PannerAudioProcessor::processBalance; break;
    default: currentProcessFunction = &PannerAudioProcessor::processLinear; break;
    }
}

void PannerAudioProcessor::calculateTargetGains(float pan, int panLaw, float& leftGain, float& rightGain)
{
    switch (panLaw)
    {
    case 0: // Linear
        leftGain = (1.0f - pan) * 0.5f;
        rightGain = (1.0f + pan) * 0.5f;
        break;
    case 1: // -3 dB Constant Power
        leftGain = std::cos((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        rightGain = std::sin((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        break;
    case 2: // Square-root
        leftGain = std::sqrt(1.0f - ((pan + 1.0f) * 0.5f));
        rightGain = std::sqrt(((pan + 1.0f) * 0.5f));
        break;
    case 3: // Balance
        leftGain = pan <= 0.0f ? 1.0f : 1.0f - pan;
        rightGain = pan >= 0.0f ? 1.0f : 1.0f + pan;
        break;
    default:
        leftGain = 0.5f;
        rightGain = 0.5f;
        break;
    }
}

void PannerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numSamples = buffer.getNumSamples();
    const int numChans = buffer.getNumChannels();

    if (numChans < 2 || numSamples == 0)
        return;

    // CPU optimization: Check for silence (Melda-style)
    float rmsLeft = buffer.getRMSLevel(0, 0, numSamples);
    float rmsRight = buffer.getRMSLevel(1, 0, numSamples);

    constexpr float silenceThreshold = 0.0001f; // -80 dB

    if (rmsLeft < silenceThreshold && rmsRight < silenceThreshold)
    {
        silentBlockCount++;

        // After several silent blocks, skip heavy processing
        if (silentBlockCount > kMaxSilentBlocks)
        {
            // Still update smoothing to prevent discontinuities
            for (int i = 0; i < numSamples; ++i)
            {
                smoothedLeftGain.getNextValue();
                smoothedRightGain.getNextValue();
            }
            return;
        }
    }
    else
    {
        silentBlockCount = 0;
    }

    // Update processing function if pan law changed
    updateProcessFunction();

    // Get current pan value
    float pan = *parameters.getRawParameterValue("PAN");
    int panLaw = (int)*parameters.getRawParameterValue("PAN_LAW");

    // Calculate target gains ONCE per block
    float targetLeftGain, targetRightGain;
    calculateTargetGains(pan, panLaw, targetLeftGain, targetRightGain);

    // Set smoothing targets
    smoothedLeftGain.setTargetValue(targetLeftGain);
    smoothedRightGain.setTargetValue(targetRightGain);

    // Update display meters (thread-safe atomic)
    currentDisplayLeftGain.store(targetLeftGain, std::memory_order_relaxed);
    currentDisplayRightGain.store(targetRightGain, std::memory_order_relaxed);

    // Call optimized processing function
    if (currentProcessFunction)
        (this->*currentProcessFunction)(buffer, numSamples);
}

// SIMD-optimized helper function
void PannerAudioProcessor::processSIMDWithSmoothing(juce::AudioBuffer<float>& buffer, int numSamples)
{
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    constexpr int simdSize = juce::dsp::SIMDRegister<float>::size();
    const int numVecSamples = numSamples - (numSamples % simdSize);

    int i = 0;

    // SIMD processing for aligned chunks
    for (; i < numVecSamples; i += simdSize)
    {
        auto vecLeft = juce::dsp::SIMDRegister<float>::fromRawArray(left + i);
        auto vecRight = juce::dsp::SIMDRegister<float>::fromRawArray(right + i);

        // Get smoothed gains for this vector
        float leftGains[simdSize];
        float rightGains[simdSize];

        for (int j = 0; j < simdSize; ++j)
        {
            leftGains[j] = smoothedLeftGain.getNextValue();
            rightGains[j] = smoothedRightGain.getNextValue();
        }

        auto vecLeftGain = juce::dsp::SIMDRegister<float>::fromRawArray(leftGains);
        auto vecRightGain = juce::dsp::SIMDRegister<float>::fromRawArray(rightGains);

        // Apply gains with SIMD multiplication
        vecLeft *= vecLeftGain;
        vecRight *= vecRightGain;

        vecLeft.copyToRawArray(left + i);
        vecRight.copyToRawArray(right + i);
    }

    // Process remaining samples (scalar)
    for (; i < numSamples; ++i)
    {
        left[i] *= smoothedLeftGain.getNextValue();
        right[i] *= smoothedRightGain.getNextValue();
    }
}

void PannerAudioProcessor::processLinear(juce::AudioBuffer<float>& buffer, int numSamples)
{
    processSIMDWithSmoothing(buffer, numSamples);
}

void PannerAudioProcessor::processConstantPower(juce::AudioBuffer<float>& buffer, int numSamples)
{
    processSIMDWithSmoothing(buffer, numSamples);
}

void PannerAudioProcessor::processSquareRoot(juce::AudioBuffer<float>& buffer, int numSamples)
{
    processSIMDWithSmoothing(buffer, numSamples);
}

void PannerAudioProcessor::processBalance(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Balance mode: independent channel control
    processSIMDWithSmoothing(buffer, numSamples);
}

juce::AudioProcessorEditor* PannerAudioProcessor::createEditor()
{
    return new PannerAudioProcessorEditor(*this);
}

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

void PannerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PannerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PannerAudioProcessor();
}