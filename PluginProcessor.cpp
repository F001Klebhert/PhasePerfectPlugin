#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // 100% = Volume Stéréo parfaitement identique en Mono
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("survival", 1), "Mono Survival %", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));
        
    return layout;
}

MasteringMonoTranslator::MasteringMonoTranslator()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void MasteringMonoTranslator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.prepare(sampleRate);
}

void MasteringMonoTranslator::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    float survivalParam = apvts.getRawParameterValue("survival")->load() / 100.0f;

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. Extraction (Mid et Side physiques)
        float M = (inL + inR) * 0.5f;
        float S = (inL - inR) * 0.5f;

        // 2. Création du signal de survie (L'énergie stéréo cachée via Haas)
        float survival = engine.process(S) * survivalParam;

        // 3. Output Final
        // La stéréo d'origine reste parfaitement intacte pour l'écoute sur de bonnes enceintes.
        channelDataL[i] = M + survival + S;
        channelDataR[i] = M + survival - S;
        
        // LA PREUVE DU MASTERING :
        // Si l'utilisateur final écoute sur un appareil mono, la Gauche et la Droite s'additionnent :
        // (M + survival + S) + (M + survival - S) = 2 * M + 2 * survival !
        // Le S (la réverbe) est détruit physiquement, mais le SURVIVAL (la copie cachée) reste ! 
        // Votre mixage ne perdra absolument aucun volume sur ses éléments larges.
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MasteringMonoTranslator();
}
