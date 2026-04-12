#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <cmath>

// Générateur de largeur par modulation temporelle (Type Roland/Yamaha)
class TransparentChorus {
public:
    void prepare(double sr) {
        sampleRate = sr;
        // Buffer de 100ms max
        buffer.assign(static_cast<size_t>(sr * 0.1), 0.0f); 
        writePos = 0;
        lfoPhase = 0.0f;
    }

    // Cette fonction retourne UNIQUEMENT le signal stéréo artificiel (le Side)
    float process(float midInput, float width) {
        buffer[writePos] = midInput;
        
        // LFO gauche : base de 15ms + 3ms de modulation lente
        float delayL = 15.0f + 3.0f * std::sin(lfoPhase);
        // LFO droite : en opposition de phase parfaite (180 degrés)
        float delayR = 15.0f + 3.0f * std::sin(lfoPhase + 3.14159265f);
        
        // Lecture avec interpolation linéaire pour ne pas dénaturer le son
        float leftDelayed = readDelay(delayL);
        float rightDelayed = readDelay(delayR);
        
        // Le signal Side est la différence entre les deux délais modulés
        float syntheticSide = (leftDelayed - rightDelayed) * 0.5f;
        
        // Vitesse du LFO très lente (0.5 Hz) pour un mouvement organique
        lfoPhase += 1.0f / (sampleRate * 2.0f); 
        if (lfoPhase >= 6.28318530718f) lfoPhase -= 6.28318530718f;
        
        writePos++;
        if (writePos >= buffer.size()) writePos = 0;

        return syntheticSide * width;
    }
    
private:
    float readDelay(float delayMs) {
        float delaySamples = delayMs * (sampleRate / 1000.0f);
        float readPos = writePos - delaySamples;
        while (readPos < 0.0f) readPos += buffer.size();
        
        int i0 = static_cast<int>(readPos);
        int i1 = (i0 + 1) % buffer.size();
        float frac = readPos - i0;
        
        // Interpolation linéaire pour garder les aigus parfaits
        return buffer[i0] + frac * (buffer[i1] - buffer[i0]);
    }
    
    std::vector<float> buffer;
    int writePos = 0;
    float sampleRate = 44100.0f;
    float lfoPhase = 0.0f;
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
    
    const juce::String getName() const override { return "Synth-Style Perfect Stereo"; }
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
    TransparentChorus chorus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTiPhasePerfectWidener)
};
