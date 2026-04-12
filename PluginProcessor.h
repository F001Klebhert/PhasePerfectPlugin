#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <algorithm>

// Décorrélateur transparent pour la réverbe récupérée
class SchroederAllPass {
public:
    SchroederAllPass() : delayBuffer(4096, 0.0f), writeIndex(0), delaySamples(100), gain(0.6f) {}
    
    void prepare(double sampleRate, float delayMs, float feedbackGain) {
        delaySamples = static_cast<int>(sampleRate * (delayMs / 1000.0f));
        gain = feedbackGain;
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writeIndex = 0;
    }
    
    float processSample(float input) {
        int readIndex = writeIndex - delaySamples;
        if (readIndex < 0) readIndex += delayBuffer.size();
        
        float delayedSample = delayBuffer[readIndex];
        float output = -gain * input + delayedSample + gain * delayedSample;
        
        delayBuffer[writeIndex] = input + gain * delayedSample;
        writeIndex++;
        if (writeIndex >= delayBuffer.size()) writeIndex = 0;
        
        return output;
    }
private:
    std::vector<float> delayBuffer;
    int writeIndex;
    int delaySamples;
    float gain;
};

class VSTiPhasePerfectWidener : public juce::AudioProcessor {
public:
    VSTiPhasePerfectWidener();
    ~VSTiPhasePerfectWidener() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    
    const juce::String getName() const override { return "Energy Preserving Width"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::AudioProcessorValueTreeState apvts;
    SchroederAllPass ap1;
    SchroederAllPass ap2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTiPhasePerfectWidener)
};
