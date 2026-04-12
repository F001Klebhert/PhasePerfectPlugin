#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Width Control. 1.0 = Original Stereo. 0.0 = Phase Perfect Mono (without losing reverb)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
        
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
    // Nombres premiers pour une décorrélation douce sans résonance
    ap1.prepare(sampleRate, 13.7f, 0.5f);
    ap2.prepare(sampleRate, 19.3f, 0.5f);
}

void VSTiPhasePerfectWidener::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    float width = apvts.getRawParameterValue("width")->load();

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. Extraction
        float mid = (inL + inR) * 0.5f;
        float side = (inL - inR) * 0.5f;

        // 2. Calcul des parts
        // Ce qui reste stéréo :
        float sideToKeep = side * width;
        // Ce qui s'apprête à être perdu (la réverbe qui disparaît normalement) :
        float sideLost = side * (1.0f - width);

        // 3. Récupération
        // On rend la réverbe perdue compatible avec le Mono
        float recoveredReverb = ap2.processSample(ap1.processSample(sideLost));

        // 4. Injection
        // On ajoute la réverbe sauvée au centre de l'image
        float newMid = mid + recoveredReverb;

        // 5. Reconstruction parfaite
        channelDataL[i] = newMid + sideToKeep;
        channelDataR[i] = newMid - sideToKeep;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VSTiPhasePerfectWidener();
}
