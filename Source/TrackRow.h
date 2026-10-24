#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include "Timeline.h"
#include "ClipComponent.h"

// La "voie" d'une piste sur la ligne de temps : elle contient les clips de la piste.
// Un clic dans une zone vide déplace la lecture ; un double-clic y ajoute un son.
class TrackLane : public juce::Component
{
public:
    TrackLane (Track& track, const TimelineScale& scale,
               juce::AudioFormatManager& formatManager, juce::AudioThumbnailCache& cache);

    // Recrée un ClipComponent pour chaque clip de la piste.
    void rebuild();

    std::function<void (double seconds)> onSeek;
    std::function<void (double seconds)> onAddClipRequested;
    std::function<void()> onClipsChanged;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    Track& track;
    const TimelineScale& scale;
    juce::AudioFormatManager& formatManager;
    juce::AudioThumbnailCache& thumbnailCache;
    juce::OwnedArray<ClipComponent> clipComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackLane)
};

// Une ligne de la vue "Édition" : le nom de la piste, un bouton pour ajouter un son, et la voie avec ses clips.
class TrackRow : public juce::Component
{
public:
    // Largeur de la colonne de gauche (nom + bouton) et marge, pour aligner la règle sur les voies.
    static constexpr int headerWidth = 150;
    static constexpr int padding = 6;

    TrackRow (int trackNumber, Track& track, const TimelineScale& scale,
              juce::AudioFormatManager& formatManager, juce::AudioThumbnailCache& cache);

    TrackLane& getLane() { return lane; }

    // Ajoute un clip et met l'affichage à jour. Renvoie false si le fichier est illisible.
    bool addClip (const juce::File& file, double startSeconds);

    // Retire tous les clips de la piste.
    void clear();

    // Met à jour le texte d'information ("2 clips") et la position des clips.
    void refresh();

    std::function<double()> getPlayheadPosition; // pour ajouter un son là où se trouve la lecture
    std::function<void()> onClipsChanged;        // un clip a été ajouté, déplacé ou supprimé

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void chooseFile (double startSeconds);

    const int trackNumber;
    Track& track;

    juce::Label numberLabel;
    juce::TextButton addButton { "Ajouter un son..." };
    juce::Label infoLabel { {}, "(vide)" };
    TrackLane lane;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackRow)
};
