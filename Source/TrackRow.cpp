#include "TrackRow.h"

//==============================================================================
// TrackLane

TrackLane::TrackLane (Track& t, const TimelineScale& s,
                      juce::AudioFormatManager& fm, juce::AudioThumbnailCache& cache)
    : track (t), scale (s), formatManager (fm), thumbnailCache (cache)
{
}

void TrackLane::rebuild()
{
    clipComponents.clear(); // détruit les anciens dessins de clips

    for (auto& clip : track.getClips())
    {
        auto* component = clipComponents.add (new ClipComponent (track, *clip, scale, formatManager, thumbnailCache));
        component->onMoved = [this] { resized(); };
        component->onChanged = [this] { if (onClipsChanged) onClipsChanged(); };

        Clip* clipPointer = clip.get();
        component->onDeleteRequested = [this, clipPointer]
        {
            // D'abord on retire le dessin, ensuite le clip lui-même (le dessin s'en sert).
            for (int i = 0; i < clipComponents.size(); ++i)
            {
                if (&clipComponents[i]->getClip() == clipPointer)
                {
                    clipComponents.remove (i);
                    break;
                }
            }

            track.removeClip (clipPointer);

            if (onClipsChanged)
                onClipsChanged();
        };

        addAndMakeVisible (component);
    }

    resized();
}

void TrackLane::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    if (! track.hasClips())
    {
        g.setColour (juce::Colours::grey);
        g.setFont (13.0f);
        g.drawText ("Double-clique ici pour ajouter un son", getLocalBounds(), juce::Justification::centred);
    }
}

void TrackLane::resized()
{
    // Chaque clip est placé selon son début et sa durée.
    for (auto* component : clipComponents)
    {
        const auto& clip = component->getClip();
        const int x = juce::roundToInt (scale.timeToX (clip.startSeconds, getWidth()));
        const int w = juce::roundToInt (scale.timeToX (clip.getLengthSeconds (track.getSampleRate()), getWidth()));
        component->setBounds (x, 2, juce::jmax (4, w), getHeight() - 4);
    }
}

void TrackLane::mouseDown (const juce::MouseEvent& e)
{
    if (onSeek)
        onSeek (scale.xToTime ((float) e.x, getWidth()));
}

void TrackLane::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (onAddClipRequested)
        onAddClipRequested (scale.xToTime ((float) e.x, getWidth()));
}

//==============================================================================
// TrackRow

TrackRow::TrackRow (int number, Track& t, const TimelineScale& scale,
                    juce::AudioFormatManager& formatManager, juce::AudioThumbnailCache& cache)
    : trackNumber (number),
      track (t),
      lane (t, scale, formatManager, cache)
{
    numberLabel.setText ("Piste " + juce::String (trackNumber), juce::dontSendNotification);
    numberLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (numberLabel);

    // Le bouton ajoute un son là où se trouve la tête de lecture.
    addButton.onClick = [this] { chooseFile (getPlayheadPosition ? getPlayheadPosition() : 0.0); };
    addAndMakeVisible (addButton);

    infoLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (infoLabel);

    lane.onAddClipRequested = [this] (double seconds) { chooseFile (seconds); };
    lane.onClipsChanged = [this]
    {
        refresh();
        if (onClipsChanged)
            onClipsChanged();
    };
    addAndMakeVisible (lane);
}

void TrackRow::chooseFile (double startSeconds)
{
    chooser = std::make_unique<juce::FileChooser> ("Choisis un son pour la piste " + juce::String (trackNumber),
                                                   juce::File{},
                                                   "*.wav;*.aif;*.aiff");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this, startSeconds] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{})
            return; // l'utilisateur a annulé

        if (addClip (file, startSeconds) && onClipsChanged)
            onClipsChanged();
    });
}

bool TrackRow::addClip (const juce::File& file, double startSeconds)
{
    if (track.addClip (file, startSeconds) == nullptr)
        return false;

    lane.rebuild();
    refresh();
    return true;
}

void TrackRow::clear()
{
    track.clear();
    lane.rebuild();
    refresh();
}

void TrackRow::refresh()
{
    const auto numClips = (int) track.getClips().size();
    infoLabel.setText (numClips == 0 ? "(vide)" : juce::String (numClips) + (numClips == 1 ? " clip" : " clips"),
                       juce::dontSendNotification);
    lane.resized();
    lane.repaint();
}

void TrackRow::paint (juce::Graphics& g)
{
    g.setColour (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).brighter (0.1f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
}

void TrackRow::resized()
{
    auto area = getLocalBounds().reduced (padding);

    // Colonne de gauche : numéro, bouton, nombre de clips
    auto header = area.removeFromLeft (headerWidth);
    numberLabel.setBounds (header.removeFromTop (22));
    addButton.setBounds (header.removeFromTop (26).reduced (2, 0));
    infoLabel.setBounds (header);

    area.removeFromLeft (padding);
    lane.setBounds (area);
}
