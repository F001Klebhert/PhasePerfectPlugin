#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>

// Filtre Biquad standard
class Biquad {
public:
    void setCoefficients(float b0, float b1, float b2, float a1, float a2) {
        this->b0 = b0; this->b1 = b1; this->b2 = b2;
        this->a1 = a1; this->a2 = a2;
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

// Crossover Linkwitz-Riley 4ème Ordre (Transparence Absolue)
class LR4Crossover {
public:
    void prepare(double sr) { sampleRate = sr; }
    
    void setFreq(float freq) {
        float w0 = 6.28318530718f * freq / sampleRate;
        float alpha = std::sin(w0) / 1.41421356237f; // Q = 0.707 (Butterworth)
        float cosw0 = std::cos(w0);
        float a0 = 1.0f + alpha;
        
        // Low Pass Biquad Coeffs
        float lp_b0 = ((1.0f - cosw0) / 2.0f) / a0;
        float lp_b1 = (1.0f - cosw0) / a0;
        float lp_b2 = ((1.0f - cosw0) / 2.0f) / a0;
        float a1 = (-2.0f * cosw0) / a0;
        float a2 = (1.0f - alpha) / a0;
        
        lp1_L.setCoefficients(lp_b0, lp_b1, lp_b2, a1, a2);
        lp2_L.setCoefficients(lp_b0, lp_b1, lp_b2, a1, a2);
        lp1_R.setCoefficients(lp_b0, lp_b1, lp_b2, a1, a2);
        lp2_R.setCoefficients(lp_b0, lp_b1, lp_b2, a1, a2);
        
        // High Pass Biquad Coeffs
        float hp_b0 = ((1.0f + cosw0) / 2.0f) / a0;
        float hp_b1 = -(1.0f + cosw0) / a0;
        float hp_b2 = ((1.0f + cosw0) / 2.0f) / a0;
        
        hp1_L.setCoefficients(hp_b0, hp_b1, hp_b2, a1, a2);
        hp2_L.setCoefficients(hp_b0, hp_b1, hp_b2, a1, a2);
        hp1_R.setCoefficients(hp_b0, hp_b1, hp_b2, a1, a2);
        hp2_R.setCoefficients(hp_b0, hp_b1, hp_b2, a1, a2);
    }
    
    void process(float inL, float inR, float& outL, float& outR, float width) {
        // Cascade de 2 filtres pour obtenir 24dB/Octave
        float lowL = lp2_L.process(lp1_L.process(inL));
        float highL = hp2_L.process(hp1_L.process(inL));
        
        float lowR = lp2_R.process(lp1_R.process(inR));
        float highR = hp2_R.process(hp1_R.process(inR));
        
        // Le noyau central parfait (Mono absolu sur les graves)
        float monoLow = (lowL + lowR) * 0.5f;
        
        // Traitement optionnel de largeur stéréo uniquement sur les aigus
        float highMid = (highL + highR) * 0.5f;
        float highSide = (highL - highR) * 0.5f;
        
        highL = highMid + (highSide * width);
        highR = highMid - (highSide * width);
        
        // Reconstruction finale
        outL = monoLow + highL;
        outR = monoLow + highR;
    }

private:
    float sampleRate = 44100.0f;
    Biquad lp1_L, lp2_L, hp1_L, hp2_L;
    Biquad lp1_R, lp2_R, hp1_R, hp2_R;
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
    
    const juce::String getName() const override { return "Transparent Mono Bass"; }
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
    LR4Crossover crossover;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTiPhasePerfectWidener)
};
