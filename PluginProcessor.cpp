#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Le bouton contrôlera la largeur de la pièce virtuelle
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Natural Stereo Width", 
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
    // 20ms est le point idéal pour que le cerveau entende une pièce, pas un écho
    delay.prepare(sampleRate, 20.0f);
    // On coupe tout ce qui est sous 250Hz dans les côtés pour garder la basse ferme
    hpf.prepare(sampleRate, 250.0f);
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

        // 1. On crée un noyau central 100% stable et intact
        float mid = (inL + inR) * 0.5f;

        // 2. On génère l'espace stéréo naturel (Effet psychoacoustique Haas)
        float space = delay.process(mid);
        space = hpf.process(space);

        // On applique le volume choisi par l'utilisateur
        space *= widthParam;

        // 3. LA MAGIE DE LA PHASE : 
        // L = Centre + Espace  |  R = Centre - Espace
        // En Mono, l'Espace s'annule à 100%. Il ne reste que le Centre pur !
        channelDataL[i] = mid + space;
        channelDataR[i] = mid - space;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VSTiPhasePerfectWidener();
}
