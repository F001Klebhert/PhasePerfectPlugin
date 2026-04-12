#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// Crossover à reconstruction parfaite (Zero Timbre Change)
class PhaseAlignCrossover {
public:
    void prepare(double sr) { sampleRate = sr; }
    
    void setFreq(float freq) {
        float dt = 1.0f / sampleRate;
        float rc = 1.0f / (6.28318530718f * freq);
        alpha = dt / (rc + dt);
    }
    
    void process(float inL, float inR, float& outL, float& outR, float highWidth) {
        // Séparation Gauche (Filtre 1 pôle)
        lpL = lpL + alpha * (inL - lpL);
        float hpL = inL - lpL; // Les aigus gauche
        
        // Séparation Droite
        lpR = lpR + alpha * (inR - lpR);
        float hpR = inR - lpR; // Les aigus droite
        
        // FORCER LA PHASE A +1.0 SUR LES GRAVES/MEDIUMS
        float perfectMonoCore = (lpL + lpR) * 0.5f;
        
        // Traitement M/S uniquement sur les aigus
        float highMid = (hpL + hpR) * 0.5f;
        float highSide = (hpL - hpR) * 0.5f;
        
        // Reconstruction totale : Core (+1 Phase) + Aigus (Stereo)
        outL = perfectMonoCore + highMid + (highSide * highWidth);
        outR = perfectMonoCore + highMid - (highSide * highWidth);
    }
    
private:
    float sampleRate = 44100.0f;
    float alpha = 0.01f;
    float lpL = 0.0f, lpR = 0.0f;
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
    
    const juce::String getName() const override { return "Mastering Phase Aligner"; }
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
    PhaseAlignCrossover crossover;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTiPhasePerfectWidener)
};
