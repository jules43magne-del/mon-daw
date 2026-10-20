#pragma once

#include <JuceHeader.h>
#include "LevelMeter.h"

// Une tranche de console de mixage, comme dans la fenêtre Mix de Pro Tools :
// nom, Mute / Solo, panoramique, fader de volume et vumètre.
// La tranche ne connaît pas l'audio : elle prévient le reste du programme via des callbacks.
class MixerStrip : public juce::Component
{
public:
    // isMaster = true : tranche Master (pas de Mute / Solo ni de panoramique).
    MixerStrip (const juce::String& name, bool isMaster);

    void setName (const juce::String& name);
    LevelMeter& getMeter() { return meter; }

    // Appelés quand l'utilisateur bouge un réglage.
    std::function<void (float)> onGainChange;
    std::function<void (float)> onPanChange;
    std::function<void (bool)> onMuteChange;
    std::function<void (bool)> onSoloChange;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    const bool isMaster;

    juce::Label nameLabel;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::Slider panKnob;
    juce::Slider fader;
    LevelMeter meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerStrip)
};
