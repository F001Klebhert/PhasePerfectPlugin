#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // 1.0 = La largeur stéréo est 100% INTACTE (Bypass visuel parfait). 
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "TrueFold Width", 
        juce::NormalisableRange<float>(0.0f, 1.5f, 0.01f), 1.0f));

    // L'intensité des Tails sauvés en Mono (0.5 = naturel, 1.0 = massif)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("tailSurvival", 1), "Tail Mono Survival", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
    return layout;
}

SymphonicApexMaster::SymphonicApexMaster()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void SymphonicApexMaster::prepareToPlay(double sampleRate, int samplesPerBlock) {
    tailDetector.prepare(sampleRate);
    delayL.prepare(sampleRate, 13.0f);
    delayR.prepare(sampleRate, 19.0f);
    
    lpfL.prepare(sampleRate); lpfL.setFreq(6000.0f, true);
    lpfR.prepare(sampleRate); lpfR.setFreq(6000.0f, true);
    hpfL.prepare(sampleRate); hpfL.setFreq(200.0f, false);
    hpfR.prepare(sampleRate); hpfR.setFreq(200.0f, false);
}

void SymphonicApexMaster::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    float width = apvts.getRawParameterValue("width")->load();
    float tailGain = apvts.getRawParameterValue("tailSurvival")->load();

    // Matrice TrueFold (#14) pour une fluidité parfaite du volume
    float angle = (1.0f - width) * (3.14159265359f / 4.0f);
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    float norm = 1.0f / (1.0f + 0.5f * ((cos_a + sin_a) - 1.0f));

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. La base fluide TrueFold (Transitoires 100% conservés)
        float baseL = (inL * cos_a + inR * sin_a) * norm;
        float baseR = (inR * cos_a + inL * sin_a) * norm;

        // 2. Détection Intelligente : On extrait l'espace, et on repère les transitoires
        float side = (baseL - baseR) * 0.5f;
        float tailMask = tailDetector.process(side);
        
        // 3. Le Miracle : On annule l'effet fantôme PENDANT les attaques !
        float safeSide = side * tailMask; 

        // 4. Seules les queues de réverbes douces passent dans la machine de survie Mono
        float phantomL = hpfL.process(lpfL.process(delayL.process(safeSide)));
        float phantomR = hpfR.process(lpfR.process(delayR.process(safeSide)));

        // 5. Reconstruction finale
        channelDataL[i] = baseL + (phantomL * tailGain);
        channelDataR[i] = baseR + (phantomR * tailGain);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SymphonicApexMaster();
}
