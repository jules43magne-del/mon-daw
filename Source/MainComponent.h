#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include "TrackRow.h"
#include "MixerStrip.h"

// La fenêtre principale :
//  - en haut, la barre de transport (Lecture / Pause / Stop et compteur de temps) ;
//  - au milieu, la vue Édition : une ligne par piste avec sa forme d'onde ;
//  - en bas, la console de mixage : une tranche par piste, plus le Master.
class MainComponent : public juce::AudioAppComponent,
                      private juce::Timer
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
    static constexpr int numTracks = 4;

    void play();
    void pause();
    void stop();
    void seekAll (double seconds);
    double getPosition() const;
    bool anyTrackLoaded() const;
    void updateTimeline();
    void timerCallback() override;

    // Audio
    juce::AudioFormatManager formatManager;      // sait lire WAV, AIFF...
    juce::OwnedArray<Track> tracks;              // OwnedArray détruit ses éléments tout seul
    juce::AudioBuffer<float> trackBuffer;        // brouillon pour mixer une piste, préparé à l'avance
    std::atomic<bool> playing { false };
    std::atomic<float> masterGain { 0.8f };

    // Barre de transport
    juce::TextButton playButton { "Lecture" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton stopButton { "Stop" };
    juce::Label timeLabel;

    // Vues
    juce::OwnedArray<TrackRow> rows;
    juce::OwnedArray<MixerStrip> strips;
    MixerStrip masterStrip { "Master", true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
