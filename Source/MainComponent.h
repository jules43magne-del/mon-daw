#pragma once

#include <JuceHeader.h>
#include "WaveformView.h"

// Le contenu de la fenêtre : boutons, fader de volume, et lecture audio.
// AudioAppComponent nous donne directement accès à la carte son.
class MainComponent : public juce::AudioAppComponent,
                      private juce::ChangeListener
{
public:
    MainComponent();
    ~MainComponent() override;

    // Partie audio : appelée par JUCE sur le "thread audio" en temps réel.
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    // Partie graphique.
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void openButtonClicked();
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void updateButtons();

    // Interface
    juce::TextButton openButton { "Ouvrir un WAV..." };
    juce::TextButton playButton { "Lecture" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton stopButton { "Stop" };
    juce::Slider volumeSlider;
    juce::Label volumeLabel { {}, "Volume" };
    juce::Label fileLabel { {}, "Aucun fichier" };

    // Audio
    juce::AudioFormatManager formatManager;                       // sait lire WAV, AIFF...
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;  // le fichier ouvert
    juce::AudioTransportSource transportSource;                   // lecture / stop / volume
    std::unique_ptr<juce::FileChooser> chooser;

    // Déclaré après formatManager et transportSource, car il les utilise.
    WaveformView waveformView { formatManager, transportSource };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
