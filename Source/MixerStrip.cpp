#include "MixerStrip.h"

MixerStrip::MixerStrip (const juce::String& name, bool master)
    : isMaster (master)
{
    nameLabel.setText (name, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (nameLabel);

    if (! isMaster)
    {
        // Mute et Solo restent enfoncés quand on clique dessus (clickingTogglesState).
        muteButton.setClickingTogglesState (true);
        muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::orange);
        muteButton.onClick = [this] { if (onMuteChange) onMuteChange (muteButton.getToggleState()); };
        addAndMakeVisible (muteButton);

        soloButton.setClickingTogglesState (true);
        soloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::yellow.darker());
        soloButton.onClick = [this] { if (onSoloChange) onSoloChange (soloButton.getToggleState()); };
        addAndMakeVisible (soloButton);

        // Panoramique : bouton rotatif de -1 (gauche) à +1 (droite). Double-clic = retour au centre.
        panKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        panKnob.setRange (-1.0, 1.0, 0.01);
        panKnob.setValue (0.0);
        panKnob.setDoubleClickReturnValue (true, 0.0);
        panKnob.setTooltip ("Panoramique");
        panKnob.onValueChange = [this] { if (onPanChange) onPanChange ((float) panKnob.getValue()); };
        addAndMakeVisible (panKnob);
    }

    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    fader.setRange (0.0, 1.0, 0.01);
    fader.setValue (0.8);
    fader.onValueChange = [this] { if (onGainChange) onGainChange ((float) fader.getValue()); };
    addAndMakeVisible (fader);

    addAndMakeVisible (meter);
}

void MixerStrip::setName (const juce::String& name)
{
    nameLabel.setText (name, juce::dontSendNotification);
}

void MixerStrip::setGain (float gain)
{
    fader.setValue (gain, juce::sendNotificationSync); // déclenche fader.onValueChange
}

void MixerStrip::setPan (float pan)
{
    panKnob.setValue (pan, juce::sendNotificationSync); // déclenche panKnob.onValueChange
}

void MixerStrip::setMute (bool shouldMute)
{
    muteButton.setToggleState (shouldMute, juce::dontSendNotification);
    if (onMuteChange)
        onMuteChange (shouldMute);
}

void MixerStrip::setSolo (bool shouldSolo)
{
    soloButton.setToggleState (shouldSolo, juce::dontSendNotification);
    if (onSoloChange)
        onSoloChange (shouldSolo);
}

void MixerStrip::paint (juce::Graphics& g)
{
    // Un fond légèrement plus clair et un cadre, pour bien séparer les tranches.
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).brighter (isMaster ? 0.2f : 0.1f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void MixerStrip::resized()
{
    auto area = getLocalBounds().reduced (6);

    nameLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (4);

    if (! isMaster)
    {
        auto buttons = area.removeFromTop (24);
        muteButton.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (2, 0));
        soloButton.setBounds (buttons.reduced (2, 0));
        area.removeFromTop (4);
        panKnob.setBounds (area.removeFromTop (44));
    }
    else
    {
        area.removeFromTop (72); // garde les faders alignés avec ceux des pistes
    }

    area.removeFromTop (4);
    auto meterArea = area.removeFromRight (16);
    meter.setBounds (meterArea.withTrimmedBottom (24)); // s'arrête au-dessus de la case du volume
    area.removeFromRight (4);
    fader.setBounds (area);
}
