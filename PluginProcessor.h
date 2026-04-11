#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

// Un délai simple pour créer un rebond acoustique naturel
class SimpleDelay {
public:
    SimpleDelay() {}
    void prepare(double sampleRate, float delayMs) {
        buffer.assign(static_cast<size_t>(sampleRate * 2.0), 0.0f);
        delaySamples = static_cast<int>(sampleRate * (delayMs / 1000.0f));
        writePos = 0;
    }
    float process(float input) {
        buffer[writePos] = input;
        int readPos = writePos - delaySamples;
        if (readPos < 0) readPos += buffer.size();
        float output = buffer[readPos];
        writePos++;
        if (writePos >= buffer.size()) writePos = 0;
        return output;
    }
private:
    std::vector<float> buffer;
    int writePos = 0;
    int delaySamples = 0;
};

// Filtre coupe-bas pour garder les basses fréquences strictement au centre
class SimpleHPF {
public:
    void prepare(double sampleRate, float cutoffFreq) {
        float dt = 1.0f / sampleRate;
        float rc = 1.0f / (6.28318530718f * cutoffFreq); // 2 * PI * Freq
        alpha = rc / (rc + dt);
        y_prev = 0.0f;
        x_prev = 0.0f;
    }
    float process(float x) {
        float y = alpha * (y_prev + x - x_prev);
        x_prev = x;
        y_prev = y;
        return y;
    }
private:
    float alpha = 0.0f;
    float y_prev = 0.0f;
    float x_prev = 0.0f;
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
    
    const juce::String getName() const override { return "Natural Phase-Perfect Stereo"; }
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
    SimpleDelay delay;
    SimpleHPF hpf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTiPhasePerfectWidener)
};
