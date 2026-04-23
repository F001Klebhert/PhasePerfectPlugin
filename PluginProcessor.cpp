#include "PluginProcessor.h"
#include <memory>
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Largeur Trigonométrique (Conserve 100% de la réverbe et des transitoires)
    // 1.0 = Bypass Parfait. 0.0 = Mono Parfait.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Acoustic Width", 
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
        
    // L'équilibre Psychoacoustique (La Loi du Centre)
    // 0.0 = La réverbe est préservée à 100% absolue, mais le centre augmente un peu en mono
    // 1.0 = Le centre est préservé à 100%, mais la réverbe baisse légèrement
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("centerComp", 1), "Center Mono Balance", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
    return layout;
}

AcousticTrueFold::AcousticTrueFold()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void AcousticTrueFold::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    float width = apvts.getRawParameterValue("width")->load();
    float comp = apvts.getRawParameterValue("centerComp")->load();

    // 1. Calcul de l'angle de Rotation de Blumlein (en radians)
    // À 1.0 (Stéréo), l'angle est 0°. À 0.0 (Mono), l'angle est 45°.
    float angle = (1.0f - width) * (3.14159265359f / 4.0f);
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);

    // 2. Calcul du gain pour équilibrer la sommation du centre
    float center_boost = cos_a + sin_a; // Vaut 1.414 en Mono pur (+3dB)
    float norm = 1.0f / (1.0f + comp * (center_boost - 1.0f));

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 3. Matrice de puissance constante (Zéro altération de phase, zéro délai)
        float outL = (inL * cos_a + inR * sin_a) * norm;
        float outR = (inR * cos_a + inL * sin_a) * norm;

        channelDataL[i] = outL;
        channelDataR[i] = outR;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AcousticTrueFold();
}
