#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <cmath>
#include <algorithm>

class BiquadHPF {
public:
    void prepare(double sr, float freq) {
        float w0 = 6.28318530718f * freq / sr;
        float alpha = std::sin(w0) / 1.41421356237f;
        float cosw0 = std::cos(w0);
        float a0 = 1.0f + alpha;
        b0 = ((1.0f + cosw0) / 2.0f) / a0;
        b1 = -(1.0f + cosw0) / a0;
        b2 = ((1.0f + cosw0) / 2.0f) / a0;
        a1 = (-2.0f * cosw0) / a0;
        a2 = (1.0f - alpha) / a0;
        x1 = x2 = y1 = y2 = 0.0f;
    }
    float process(float x) {
        float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }
private:
    float b0=0, b1=0, b2=0, a1=0, a2=0;
    float x1=0, x2=0, y1=0, y2=0;
};

class SchroederAllPass {
public:
    SchroederAllPass() : delayBuffer(4096, 0.0f) {}
    void prepare(double sr, float delayMs, float feedbackGain) {
        delaySamples = static_cast<int>(sr * (delayMs / 1000.0f));
        gain = feedbackGain;
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePos = 0;
    }
    float process(float input) {
        int readPos = writePos - delaySamples;
        if (readPos < 0) readPos += delayBuffer.size();
        float delayed = delayBuffer[readPos];
        float out = -gain * input + delayed + gain * delayed;
        delayBuffer[writePos] = input + gain * delayed;
        writePos++;
        if (writePos >= delayBuffer.size()) writePos = 0;
        return out;
    }
private:
    std::vector<float> delayBuffer;
    int writePos = 0;
    int delaySamples = 0;
    float gain = 0.6f;
};

// Le moteur qui cache l'énergie stéréo dans le centre
class MonoSurvivalEngine {
public:
    void prepare(double sr) {
        hpf.prepare(sr, 150.0f); // Ne pas polluer la basse Mono
        ap1.prepare(sr, 7.1f, 0.6f);
        ap2.prepare(sr, 11.3f, 0.6f);
        ap3.prepare(sr, 13.7f, 0.6f);
    }
    float process(float sideSignal) {
        // HPF -> Diffusion Haas totale
        return ap3.process(ap2.process(ap1.process(hpf.process(sideSignal))));
    }
private:
    BiquadHPF hpf;
    SchroederAllPass ap1, ap2, ap3;
};

class MasteringMonoTranslator : public juce::AudioProcessor {
public:
    MasteringMonoTranslator();
    ~MasteringMonoTranslator() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Mastering Mono Translator"; }
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
    MonoSurvivalEngine engine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasteringMonoTranslator)
};
