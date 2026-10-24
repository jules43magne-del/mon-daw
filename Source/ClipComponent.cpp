#include "ClipComponent.h"

ClipComponent::ClipComponent (Track& t, Clip& c, const TimelineScale& s,
                              juce::AudioFormatManager& formatManager, juce::AudioThumbnailCache& cache)
    : track (t), clip (c), scale (s),
      thumbnail (512, formatManager, cache)
{
    thumbnail.addChangeListener (this);
    thumbnail.setSource (new juce::FileInputSource (clip.file));
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

ClipComponent::~ClipComponent()
{
    thumbnail.removeChangeListener (this);
}

void ClipComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // Le fond du clip, plus clair pendant qu'on le déplace.
    g.setColour (juce::Colours::seagreen.withAlpha (dragging ? 0.9f : 0.6f));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colours::lightgreen);
    g.drawRoundedRectangle (area.reduced (0.5f), 4.0f, 1.0f);

    // La forme d'onde, sous la barre de titre.
    auto waveArea = getLocalBounds().reduced (2).withTrimmedTop (16);
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    thumbnail.drawChannels (g, waveArea, 0.0, thumbnail.getTotalLength(), 1.0f);

    // Le nom du fichier en haut à gauche.
    g.setColour (juce::Colours::white);
    g.setFont (12.0f);
    g.drawText (clip.file.getFileNameWithoutExtension(), getLocalBounds().reduced (4, 0).removeFromTop (16),
                juce::Justification::centredLeft, true);
}

void ClipComponent::mouseDown (const juce::MouseEvent& e)
{
    // Clic droit : un petit menu.
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Supprimer le clip");

        // SafePointer : si le clip disparaît entre-temps, on ne touche plus à rien.
        juce::Component::SafePointer<ClipComponent> safeThis (this);
        menu.showMenuAsync (juce::PopupMenu::Options(), [safeThis] (int choice)
        {
            if (safeThis == nullptr || choice != 1 || safeThis->onDeleteRequested == nullptr)
                return;

            // On copie le callback avant de l'appeler : il va détruire ce ClipComponent (et son onDeleteRequested).
            auto deleteCallback = safeThis->onDeleteRequested;
            deleteCallback();
        });
        return;
    }

    dragStartScreenX = e.getScreenX();
    dragStartSeconds = clip.startSeconds;
    dragging = true;
    repaint();
}

void ClipComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging || getParentComponent() == nullptr)
        return;

    // On convertit le déplacement de la souris (en pixels) en secondes.
    // On utilise la position à l'écran, car le clip lui-même bouge pendant qu'on le glisse.
    const int laneWidth = getParentComponent()->getWidth();
    const double secondsPerPixel = scale.lengthSeconds / juce::jmax (1, laneWidth);
    const double newStart = dragStartSeconds + (e.getScreenX() - dragStartScreenX) * secondsPerPixel;

    track.moveClip (&clip, newStart);

    if (onMoved)
        onMoved();
}

void ClipComponent::mouseUp (const juce::MouseEvent&)
{
    if (! dragging)
        return;

    dragging = false;
    repaint();

    if (onChanged)
        onChanged();
}
