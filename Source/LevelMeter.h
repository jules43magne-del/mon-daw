#pragma once

#include <JuceHeader.h>

// Vumètre stéréo : deux barres verticales (gauche / droite) qui montrent le niveau du son.
// Le thread audio y écrit le niveau avec setLevel(), et l'interface le dessine 30 fois par seconde.
class LevelMeter : public juce::Component,
                   private juce::Timer
{
public:
    LevelMeter();

    // Appelée depuis le thread audio : doit être très rapide, sans allocation ni attente.
    void setLevel (int channel, float peak);

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    static constexpr int numChannels = 2;

    // std::atomic permet au thread audio d'écrire pendant que l'interface lit, sans risque.
    std::atomic<float> incomingPeak[numChannels] {};

    // Niveau affiché, qui redescend doucement (comme l'aiguille d'un vrai vumètre).
    float displayedPeak[numChannels] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
