#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Contrôle d'intensité de l'effet
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Extra Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
    return layout;
}

VSTiPhasePerfectWidener::VSTiPhasePerfectWidener()
    // C'EST ICI QUE MANQUAIENT LES "juce::" !
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void VSTiPhasePerfectWidener::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Préparation des filtres avec des temps en millisecondes basés sur des nombres premiers
    apf1.prepare(sampleRate, 2.3f, 0.6f);
    apf2.prepare(sampleRate, 3.7f, 0.6f);
    apf3.prepare(sampleRate, 5.1f, 0.6f);
}

void VSTiPhasePerfectWidener::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    // Récupérer la valeur du bouton de largeur sur l'interface
    float widthParam = apvts.getRawParameterValue("width")->load();

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. SÉPARATION MID / SIDE
        float mid = (inL + inR) * 0.5f;
        float originalSide = (inL - inR) * 0.5f;

        // 2. GÉNÉRATION DE STÉRÉO ARTIFICIELLE À PARTIR DU CENTRE
        float artificialSide = apf1.processSample(mid);
        artificialSide = apf2.processSample(artificialSide);
        artificialSide = apf3.processSample(artificialSide);

        // Application du volume
        artificialSide *= widthParam;

        // 3. RECOMBINAISON PARFAITE
        float totalSide = originalSide + artificialSide;

        channelDataL[i] = mid + totalSide;
        channelDataR[i] = mid - totalSide;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VSTiPhasePerfectWidener();
}
