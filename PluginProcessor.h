#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <cmath>

// Détecteur de transitoires (Sépare les attaques des réverbes)
class TailDetector {
public:
    void prepare(double sr) {
        fastAtt = 1.0f - std::exp(-1.0f / (sr * 0.001f)); // 1ms
        fastRel = 1.0f - std::exp(-1.0f / (sr * 0.015f)); // 15ms
        slowAtt = 1.0f - std::exp(-1.0f / (sr * 0.010f)); // 10ms
        slowRel = 1.0f - std::exp(-1.0f / (sr * 0.050f)); // 50ms
        fastEnv = 0.0f; slowEnv = 0.0f; smoothMask = 1.0f;
        maskSmoothCoeff = 1.0f - std::exp(-1.0f / (sr * 0.005f)); 
    }
    float process(float in) {
        float absIn = std::abs(in);
        fastEnv += (absIn > fastEnv ? fastAtt : fastRel) * (absIn - fastEnv);
        slowEnv += (absIn > slowEnv ? slowAtt : slowRel) * (absIn - slowEnv);
        
        float ratio = fastEnv / (slowEnv + 1e-5f);
        float targetMask = 1.0f;
        
        // Si c'est une attaque brutale (transitoire), on coupe l'effet (Mask = 0)
        if (ratio > 2.0f) {
            targetMask = 0.0f;
        } else if (ratio > 1.2f) {
            targetMask = 1.0f - ((ratio - 1.2f) * 1.25f);
        }
        
        smoothMask += maskSmoothCoeff * (targetMask - smoothMask);
        return smoothMask; // Renvoie 1.0 sur les Tails, 0.0 sur les Attaques
    }
private:
    float fastAtt, fastRel, slowAtt, slowRel;
    float fastEnv, slowEnv, smoothMask, maskSmoothCoeff;
};

// Délai fractionnel pur
class FractionalDelay {
public:
    void prepare(double sr, float delayMs) {
        buffer.assign(static_cast<size_t>(sr * 0.1), 0.0f);
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
};

// Filtre doux pour les tails
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
    float sampleRate = 44100.0f, alpha = 0.0f, z1 = 0.0f;
    bool lp = true;
};

class SymphonicApexMaster : public juce::AudioProcessor {
public:
    SymphonicApexMaster();
    ~SymphonicApexMaster() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Symphonic Apex Master"; }
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
    TailDetector tailDetector;
    FractionalDelay delayL, delayR;
    WarmFilter lpfL, lpfR, hpfL, hpfR;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SymphonicApexMaster)
};
