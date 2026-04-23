#include "PluginProcessor.h"
#include <memory>

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Le taux d'injection de la réverbe secrète (Mono Survival)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("phantom", 1), "Phantom Mono Survival", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
    // Permet de forcer la largeur stéréo d'origine (1.0 = Intacte)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
        
    return layout;
}

AcousticMirageMaster::AcousticMirageMaster()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void AcousticMirageMaster::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Délais de salle naturels (13ms et 19ms évitent tout effet métallique)
    delayL.prepare(sampleRate, 13.0f);
    delayR.prepare(sampleRate, 19.0f);
    
    // Filtres pour rendre l'écho fantôme parfaitement doux
    lpfL.prepare(sampleRate); lpfL.setFreq(6000.0f, true);
    lpfR.prepare(sampleRate); lpfR.setFreq(6000.0f, true);
    hpfL.prepare(sampleRate); hpfL.setFreq(200.0f, false);
    hpfR.prepare(sampleRate); hpfR.setFreq(200.0f, false);
}

void AcousticMirageMaster::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);

    float phantomGain = apvts.getRawParameterValue("phantom")->load();
    float widthGain = apvts.getRawParameterValue("width")->load();

    for (int i = 0; i < numSamples; ++i) {
        float inL = channelDataL[i];
        float inR = channelDataR[i];

        // 1. Matrice transparente
        float mid = (inL + inR) * 0.5f;
        float side = (inL - inR) * 0.5f;

        // 2. Création de l'Espace Fantôme (Acoustic Mirage)
        // On copie la réverbe (side), on la retarde, on la réchauffe
        float phantomL = hpfL.process(lpfL.process(delayL.process(side)));
        float phantomR = hpfR.process(lpfR.process(delayR.process(side)));

        // 3. Application du volume fantôme
        phantomL *= phantomGain;
        phantomR *= phantomGain;

        // 4. Mixage Final (Le secret est ici)
        // La stéréo est TOTALEMENT conservée (mid + side*width).
        // On y ajoute simplement le mirage fantôme en fond de salle.
        channelDataL[i] = mid + (side * widthGain) + phantomL;
        channelDataR[i] = mid - (side * widthGain) + phantomR;
        
        /* LA PREUVE MATHÉMATIQUE DU MIRACLE :
           En écoute Mono (L + R) :
           (mid + side + phantomL) + (mid - side + phantomR)
           = 2*mid + phantomL + phantomR
           Le side d'origine s'annule toujours (physique respectée),
           MAIS les réverbes fantômes (phantomL et phantomR) SURVIVENT !
           Le son mono est luxuriant, et la stéréo d'origine était intacte.
        */
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AcousticMirageMaster();
}
