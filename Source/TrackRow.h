#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include "WaveformView.h"

// Une ligne de la vue "Édition" : bouton pour charger un fichier, nom, et forme d'onde de la piste.
class TrackRow : public juce::Component
{
public:
    TrackRow (int trackNumber, Track& track, juce::AudioFormatManager& formatManager);

    WaveformView& getWaveform() { return waveform; }

    // Appelé après le chargement d'un fichier, pour que la fenêtre se mette à jour.
    std::function<void()> onFileLoaded;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void chooseFile();

    const int trackNumber;
    Track& track;

    juce::Label numberLabel;
    juce::TextButton loadButton { "Charger..." };
    juce::Label fileLabel { {}, "(vide)" };
    WaveformView waveform;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackRow)
};
