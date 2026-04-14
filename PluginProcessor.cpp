#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Gain du Centre (Mid) - Les instruments principaux
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("midGain", 1), "Mid Gain (dB)", 
        juce::NormalisableRange<float>(-12.0f, 6.0f, 0.1f), 0.0f));
        
    // Gain des Côtés (Side) - La réverbe, l'espace
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("sideGain", 1), "Side Gain (dB)", 
        juce::NormalisableRange<float>(-12.0f, 6.0f, 0.1f), 0.0f));

    // Nettoyage des graves sur les côtés (Pour la survie Mono des contrebasses)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("sideHPF", 1), "Side HPF (Hz)", 
        juce::NormalisableRange<float>(20.0f, 300.0f, 1.0f, 0.5f), 100.0f));
        
    return layout;
}

OrchestralConsole::OrchestralConsole()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void OrchestralConsole::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sideHPF.prepare(sampleRate);
}

void OrchestralConsole::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    // Convertir les dB en multiplicateurs de volume pur (Transparence totale)
    float midGainDb = apvts.getRawParameterValue("midGain")->load();
    float sideGainDb = apvts.getRawParameterValue("sideGain")->load();
    float midLinear = std::pow(10.0f, midGainDb / 20.0f);
    float sideLinear = std::pow(10.0f, sideGainDb / 20.0f);
    
    float hpfFreq = apvts.getRawParameterValue("sideHPF")->load();
    sideHPF.setCutoff(hpfFreq);

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. Encodage Mid/Side (Mathématique pure, ZERO déphasage)
        float mid = (inL + inR) * 0.5f;
        float side = (inL - inR) * 0.5f;

        // 2. Traitement d'égalisation et de gain
        // On enlève les extrêmes graves du côté pour garantir l'impact en Mono
        side = sideHPF.process(side);

        mid *= midLinear;
        side *= sideLinear;

        // 3. Décodage vers les enceintes
        channelDataL[i] = mid + side;
        channelDataR[i] = mid - side;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OrchestralConsole();
}
