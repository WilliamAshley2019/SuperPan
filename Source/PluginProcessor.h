#pragma once

#include <JuceHeader.h>

class PannerAudioProcessor : public juce::AudioProcessor
{
public:
    PannerAudioProcessor();
    ~PannerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    // Expose current gains for meter display
    float getCurrentLeftGain() const { return currentDisplayLeftGain.load(); }
    float getCurrentRightGain() const { return currentDisplayRightGain.load(); }

private:
    // SIMD-optimized processing function pointer
    using ProcessFunction = void (PannerAudioProcessor::*)(juce::AudioBuffer<float>&, int);
    ProcessFunction currentProcessFunction = nullptr;

    // Individual pan law processing methods (branch-free)
    void processLinear(juce::AudioBuffer<float>& buffer, int numSamples);
    void processConstantPower(juce::AudioBuffer<float>& buffer, int numSamples);
    void processSquareRoot(juce::AudioBuffer<float>& buffer, int numSamples);
    void processBalance(juce::AudioBuffer<float>& buffer, int numSamples);

    // Helper for SIMD processing with gain smoothing
    void processSIMDWithSmoothing(juce::AudioBuffer<float>& buffer, int numSamples);

    // Update processing function based on pan law selection
    void updateProcessFunction();

    // Calculate target gains from pan value and law
    void calculateTargetGains(float pan, int panLaw, float& leftGain, float& rightGain);

    // Smoothed gain values
    juce::LinearSmoothedValue<float> smoothedLeftGain;
    juce::LinearSmoothedValue<float> smoothedRightGain;

    // Atomic values for thread-safe meter display
    std::atomic<float> currentDisplayLeftGain{ 0.5f };
    std::atomic<float> currentDisplayRightGain{ 0.5f };

    // Cached parameter values to detect changes
    float lastPan = 0.0f;
    int lastPanLaw = -1;

    // Sample rate
    double currentSampleRate = 44100.0;

    // CPU optimization: Skip processing if no audio input
    bool shouldProcess = true;
    int silentBlockCount = 0;
    static constexpr int kMaxSilentBlocks = 10; // After 10 silent blocks, reduce processing

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PannerAudioProcessor)
};