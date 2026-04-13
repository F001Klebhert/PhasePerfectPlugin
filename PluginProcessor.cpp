#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Bouton ON/OFF pour comparer le miracle de la sommation
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("enable", 1), "Orthogonal Engine ON", true));
        
    return layout;
}

OrthogonalMaster::OrthogonalMaster()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void OrthogonalMaster::prepareToPlay(double sampleRate, int samplesPerBlock) {
    hilbert.prepare(sampleRate);
}

void OrthogonalMaster::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    bool enabled = apvts.getRawParameterValue("enable")->load() > 0.5f;

    if (!enabled) return; // Si désactivé, le son passe tel quel (Bypass)

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];
        float outL = 0.0f, outR = 0.0f;

        // Le moteur force l'angle mathématique à 90 degrés
        hilbert.process(inL, inR, outL, outR);

        channelDataL[i] = outL;
        channelDataR[i] = outR;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OrthogonalMaster();
}
