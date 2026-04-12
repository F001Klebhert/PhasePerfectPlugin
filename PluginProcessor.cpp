#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
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
    chorus.prepare(sampleRate);
}

void VSTiPhasePerfectWidener::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    float widthParam = apvts.getRawParameterValue("width")->load();

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. SÉPARATION MID/SIDE INTELLIGENTE
        // On extrait le centre absolu (Mid)
        float mid = (inL + inR) * 0.5f;
        // On extrait la stéréo déjà existante de votre VSTi (Side)
        float originalSide = (inL - inR) * 0.5f;

        // 2. GÉNÉRATION DE LA LARGEUR SANS TOUCHER AU TIMBRE
        // Le générateur n'écoute que le centre et crée une "largeur de synthèse"
        float syntheticSide = chorus.process(mid, widthParam);

        // 3. ADDITION ET RECOMBINAISON
        // On additionne la stéréo d'origine de votre VSTi avec notre nouvelle stéréo
        float totalSide = originalSide + syntheticSide;

        // Formule de reconstruction pure :
        channelDataL[i] = mid + totalSide;
        channelDataR[i] = mid - totalSide;
        
        // LA PREUVE MATHÉMATIQUE EN MONO :
        // Si les enceintes s'additionnent (Mono) -> Left + Right
        // (mid + totalSide) + (mid - totalSide) = 2 * mid
        // La stéréo d'origine et la nouvelle stéréo s'annulent complètement.
        // Il ne reste que le Mid pur. AUCUN comb-filtering, AUCUN changement de timbre !
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VSTiPhasePerfectWidener();
}
