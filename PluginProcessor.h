#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>

// Filtre ultra-transparent (State Variable Filter) pour les graves du Side
class TransparentHPF {
public:
    void prepare(double sr) { sampleRate = sr; }
    void setCutoff(float freq) {
        float w0 = 6.28318530718f * freq / sampleRate;
        g = std::tan(w0 / 2.0f);
        R = 1.0f; // Q = 0.5 (Très doux, aucune résonance)
    }
    float process(float x) {
        float hp = (x - (g + R)*s1 - s2) / (1.0f + g*(g + R));
        float bp = g * hp + s1;
        s1 = g * hp + bp;
        s2 = g * bp + s2;
        return hp; // On ne garde que les aigus (High Pass)
    }
private:
    float sampleRate = 44100.0f;
    float g = 0.0f, R = 1.0f;
    float s1 = 0.0f, s2 = 0.0f;
};

class OrchestralConsole : public juce::AudioProcessor {
public:
    OrchestralConsole();
    ~OrchestralConsole() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Pure Orchestral M/S"; }
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
    TransparentHPF sideHPF;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestralConsole)
};
