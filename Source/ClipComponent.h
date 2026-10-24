#pragma once

#include <JuceHeader.h>
#include "Clip.h"
#include "Track.h"
#include "Timeline.h"

// Le dessin d'un clip sur la ligne de temps : un rectangle avec sa forme d'onde et son nom.
// On peut le faire glisser à gauche ou à droite pour changer le moment où il démarre,
// et le supprimer avec un clic droit.
class ClipComponent : public juce::Component,
                      private juce::ChangeListener
{
public:
    ClipComponent (Track& track, Clip& clip, const TimelineScale& scale,
                   juce::AudioFormatManager& formatManager, juce::AudioThumbnailCache& cache);
    ~ClipComponent() override;

    Clip& getClip() { return clip; }

    std::function<void()> onMoved;            // pendant le glisser, pour se repositionner
    std::function<void()> onChanged;          // à la fin d'un déplacement
    std::function<void()> onDeleteRequested;  // clic droit > Supprimer

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

    Track& track;
    Clip& clip;
    const TimelineScale& scale;
    juce::AudioThumbnail thumbnail;

    // Ce qu'on retient au moment du clic, pour calculer le déplacement pendant le glisser.
    int dragStartScreenX = 0;
    double dragStartSeconds = 0.0;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipComponent)
};
