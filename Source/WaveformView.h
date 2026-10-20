#pragma once

#include <JuceHeader.h>

// Affiche la forme d'onde du fichier chargé et une tête de lecture.
// Un clic (ou un glisser) dans la forme d'onde déplace la lecture à cet endroit.
class WaveformView : public juce::Component,
                     private juce::ChangeListener,
                     private juce::Timer
{
public:
    WaveformView (juce::AudioFormatManager& formatManager,
                  juce::AudioTransportSource& transportSource);
    ~WaveformView() override;

    // À appeler quand un nouveau fichier est ouvert.
    void setFile (const juce::File& file);

    // Durée totale affichée (en secondes), commune à toutes les pistes pour qu'elles soient alignées.
    // 0 = utiliser la durée du fichier.
    void setTimelineLength (double seconds);

    // Si défini, appelé lors d'un clic à la place de déplacer seulement cette piste.
    std::function<void (double seconds)> onSeek;

    // Si défini, donne la position de lecture commune à toutes les pistes (en secondes).
    std::function<double()> getPosition;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void timerCallback() override;
    void seekToX (int x);
    double getTimelineLength() const;

    juce::AudioTransportSource& transportSource;

    // AudioThumbnail lit le fichier en arrière-plan et en garde une version réduite pour l'affichage.
    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail;

    double timelineLength = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformView)
};
