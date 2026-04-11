#include "PluginProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Contrôle d'intensité de l'effet
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Extra Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
    return layout;
}

VSTiPhasePerfectWidener::VSTiPhasePerfectWidener()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void VSTiPhasePerfectWidener::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Préparation des filtres avec des temps en millisecondes basés sur des nombres premiers
    // pour éviter les résonances métalliques lors de notes tenues (pads/strings).
    apf1.prepare(sampleRate, 2.3f, 0.6f);
    apf2.prepare(sampleRate, 3.7f, 0.6f);
    apf3.prepare(sampleRate, 5.1f, 0.6f);
}

void VSTiPhasePerfectWidener::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    // Récupérer la valeur du bouton de largeur sur l'interface (0% à 100%)
    float widthParam = apvts.getRawParameterValue("width")->load();

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. SÉPARATION MID / SIDE (Centre et Côtés)
        // Le Mid contient tout ce qui est identique à gauche et à droite.
        float mid = (inL + inR) * 0.5f;
        // Le Side original contient la stéréo déjà existante de votre VSTi.
        float originalSide = (inL - inR) * 0.5f;

        // 2. GÉNÉRATION DE STÉRÉO ARTIFICIELLE À PARTIR DU CENTRE
        // On ne traite que le Mid. Cela laisse les effets natifs de votre VSTi intacts.
        float artificialSide = apf1.processSample(mid);
        artificialSide = apf2.processSample(artificialSide);
        artificialSide = apf3.processSample(artificialSide);

        // On applique le volume choisi par l'utilisateur
        artificialSide *= widthParam;

        // 3. RECOMBINAISON
        // On additionne la stéréo originale et la stéréo artificielle
        float totalSide = originalSide + artificialSide;

        // On recrée la gauche et la droite avec la formule mathématique magique.
        // Left = Mid + Side
        // Right = Mid - Side
        channelDataL[i] = mid + totalSide;
        channelDataR[i] = mid - totalSide;
        
        // POURQUOI C'EST PARFAIT EN MONO ?
        // Si le système joue en mono, il fera : L + R
        // (mid + totalSide) + (mid - totalSide) = 2 * mid (Les "totalSide" s'annulent)
        // Le son reste totalement plein, sans aucun son creux !
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VSTiPhasePerfectWidener();
}
