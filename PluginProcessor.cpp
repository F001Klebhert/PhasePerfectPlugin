#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Fréquence en dessous de laquelle le son devient 100% Mono (Phase +1)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cutoff", 1), "Mono Phase Freq (Hz)", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.5f), 800.0f));
        
    // Largeur stéréo préservée (ou amplifiée) pour les aigus
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "High Freq Width", 
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
        
    return layout;
}

VSTiPhasePerfectWidener::VSTiPhasePerfectWidener()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void VSTiPhasePerfectWidener::prepareToPlay(double sampleRate, int samplesPerBlock) {
    crossover.prepare(sampleRate);
}

void VSTiPhasePerfectWidener::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    // Récupérer les réglages des boutons
    float cutoffFreq = apvts.getRawParameterValue("cutoff")->load();
    float widthParam = apvts.getRawParameterValue("width")->load();

    // Mettre à jour la fréquence cible du correcteur
    crossover.setFreq(cutoffFreq);

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];
        float outL = 0.0f, outR = 0.0f;

        // Le moteur fait le calcul de séparation de phase sans dénaturer le timbre
        crossover.process(inL, inR, outL, outR, widthParam);

        channelDataL[i] = outL;
        channelDataR[i] = outR;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VSTiPhasePerfectWidener();
}
