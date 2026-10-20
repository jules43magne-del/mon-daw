#include "LevelMeter.h"

namespace
{
    constexpr float minDecibels = -60.0f; // en dessous, la barre est vide

    // Convertit un niveau (0 à 1) en hauteur proportionnelle (0 à 1), sur une échelle en décibels.
    float levelToProportion (float peak)
    {
        const float db = juce::Decibels::gainToDecibels (peak, minDecibels);
        return juce::jmap (db, minDecibels, 0.0f, 0.0f, 1.0f);
    }
}

LevelMeter::LevelMeter()
{
    startTimerHz (30);
}

void LevelMeter::setLevel (int channel, float peak)
{
    if (channel < 0 || channel >= numChannels)
        return;

    // On garde le plus fort pic reçu depuis le dernier affichage.
    float previous = incomingPeak[channel].load();
    while (peak > previous && ! incomingPeak[channel].compare_exchange_weak (previous, peak))
    {
    }
}

void LevelMeter::timerCallback()
{
    for (int ch = 0; ch < numChannels; ++ch)
    {
        // Récupère le pic et remet le compteur à zéro pour la prochaine image.
        const float peak = incomingPeak[ch].exchange (0.0f);

        // Monte instantanément, redescend doucement.
        displayedPeak[ch] = juce::jmax (peak, displayedPeak[ch] * 0.85f);
    }

    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    const int barWidth = (area.getWidth() - 4) / numChannels;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto bar = area.removeFromLeft (barWidth);
        area.removeFromLeft (4);

        g.setColour (juce::Colours::black);
        g.fillRect (bar);

        // Dégradé vert (bas) → jaune → rouge (haut, proche de la saturation).
        juce::ColourGradient gradient (juce::Colours::red, 0.0f, (float) bar.getY(),
                                       juce::Colours::green, 0.0f, (float) bar.getBottom(), false);
        gradient.addColour (0.25, juce::Colours::yellow);
        g.setGradientFill (gradient);

        const int height = juce::roundToInt (levelToProportion (displayedPeak[ch]) * (float) bar.getHeight());
        g.fillRect (bar.removeFromBottom (height));
    }
}
