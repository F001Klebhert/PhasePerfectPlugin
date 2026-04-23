#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

// Délai fractionnel de très haute qualité (Zéro artefact métallique)
class FractionalDelay {
public:
    void prepare(double sr, float delayMs) {
        sampleRate = sr;
        buffer.assign(static_cast<size_t>(sr * 0.1), 0.0f); // 100ms max
        writePos = 0;
        delaySamples = delayMs * (static_cast<float>(sr) / 1000.0f);
    }
    float process(float input) {
        buffer[writePos] = input;
        float readPos = static_cast<float>(writePos) - delaySamples;
        if (readPos < 0.0f) readPos += static_cast<float>(buffer.size());

        int i0 = static_cast<int>(readPos);
        int i1 = (i0 + 1) % buffer.size();
        float frac = readPos - static_cast<float>(i0);

        float out = buffer[i0] + frac * (buffer[i1] - buffer[i0]);

        writePos++;
        if (writePos >= buffer.size()) writePos = 0;
        return out;
    }
private:
    std::vector<float> buffer;
    int writePos = 0;
    float delaySamples = 0.0f;
    float sampleRate = 44100.0f;
};

// Filtre analogique très doux pour réchauffer la réverbe fantôme
class WarmFilter {
public:
    void prepare(double sr) { sampleRate = sr; z1 = 0.0f; }
    void setFreq(float freq, bool isLowPass) {
        float w0 = 6.28318530718f * freq / static_cast<float>(sampleRate);
        alpha = w0 / (1.0f + w0);
        lp = isLowPass;
    }
    float process(float in) {
        z1 = z1 + alpha * (in - z1);
        return lp ? z1 : (in - z1);
    }
private:
    float sampleRate = 44100.0f;
    float alpha = 0.0f;
    float z1 = 0.0f;
    bool lp = true;
};

class AcousticMirageMaster : public juce::AudioProcessor {
public:
    AcousticMirageMaster();
    ~AcousticMirageMaster() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Acoustic Mirage Master"; }
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
    FractionalDelay delayL, delayR;
    WarmFilter lpfL, lpfR, hpfL, hpfR;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcousticMirageMaster)
};
