#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Fréquence par défaut à 150 Hz (La norme du Mastering pour sauver la réverbe)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cutoff", 1), "Mono Bass Freq (Hz)", 
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f), 150.0f));
        
    // Largeur stéréo. Par défaut à 1.0 pour un timbre EXACTEMENT inaltéré.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Stereo Width", 
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

    float cutoffFreq = apvts.getRawParameterValue("cutoff")->load();
    float widthParam = apvts.getRawParameterValue("width")->load();

    crossover.setFreq(cutoffFreq);

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];
        float outL = 0.0f, outR = 0.0f;

        // Le moteur Linkwitz-Riley garantit un timbre flat sans comb filtering
        crossover.process(inL, inR, outL, outR, widthParam);

        channelDataL[i] = outL;
        channelDataR[i] = outR;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VSTiPhasePerfectWidener();
}
