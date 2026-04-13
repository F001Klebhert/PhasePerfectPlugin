#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>

// Filtre All-Pass du 1er Ordre pour faire tourner la phase sans toucher au volume
class FirstOrderAllPass {
public:
    void prepare(double sr, float freq) {
        float tan_val = std::tan(3.14159265359f * freq / sr);
        c = (tan_val - 1.0f) / (tan_val + 1.0f);
        x1 = 0.0f;
        y1 = 0.0f;
    }
    float process(float x) {
        float y = c * x + x1 - c * y1;
        x1 = x;
        y1 = y;
        return y;
    }
private:
    float c = 0.0f;
    float x1 = 0.0f, y1 = 0.0f;
};

// Réseau de Hilbert : Force la Gauche et la Droite à être écartées de 90°
class HilbertOrthogonalNetwork {
public:
    void prepare(double sr) {
        // Fréquences magiques pour le canal GAUCHE
        apL1.prepare(sr, 14.32f);
        apL2.prepare(sr, 112.55f);
        apL3.prepare(sr, 804.53f);
        apL4.prepare(sr, 5013.3f);

        // Fréquences magiques pour le canal DROIT
        apR1.prepare(sr, 34.09f);
        apR2.prepare(sr, 276.01f);
        apR3.prepare(sr, 1980.0f);
        apR4.prepare(sr, 15300.0f);
    }
    
    void process(float inL, float inR, float& outL, float& outR) {
        outL = apL4.process(apL3.process(apL2.process(apL1.process(inL))));
        outR = apR4.process(apR3.process(apR2.process(apR1.process(inR))));
    }
private:
    FirstOrderAllPass apL1, apL2, apL3, apL4;
    FirstOrderAllPass apR1, apR2, apR3, apR4;
};

class OrthogonalMaster : public juce::AudioProcessor {
public:
    OrthogonalMaster();
    ~OrthogonalMaster() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Orthogonal Mono Sum"; }
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
    HilbertOrthogonalNetwork hilbert;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrthogonalMaster)
};
