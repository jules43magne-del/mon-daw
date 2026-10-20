#include "WaveformView.h"

WaveformView::WaveformView (juce::AudioFormatManager& formatManager,
                            juce::AudioTransportSource& transport)
    : transportSource (transport),
      thumbnail (512, formatManager, thumbnailCache) // 512 échantillons résumés en un point
{
    thumbnail.addChangeListener (this);
    startTimerHz (30); // rafraîchit la tête de lecture 30 fois par seconde
}

WaveformView::~WaveformView()
{
    thumbnail.removeChangeListener (this);
}

void WaveformView::setFile (const juce::File& file)
{
    thumbnail.setSource (new juce::FileInputSource (file));
}

void WaveformView::setTimelineLength (double seconds)
{
    timelineLength = seconds;
    repaint();
}

double WaveformView::getTimelineLength() const
{
    return timelineLength > 0.0 ? timelineLength : thumbnail.getTotalLength();
}

void WaveformView::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();

    g.setColour (juce::Colours::black);
    g.fillRect (area);

    if (thumbnail.getNumChannels() == 0)
    {
        g.setColour (juce::Colours::grey);
        g.drawFittedText ("Aucun fichier",
                          area, juce::Justification::centred, 1);
    }
    else
    {
        // La forme d'onde, tous les canaux superposés.
        g.setColour (juce::Colours::lightgreen);
        thumbnail.drawChannels (g, area.reduced (2), 0.0, getTimelineLength(), 1.0f);
    }

    // La tête de lecture : une ligne verticale à la position actuelle.
    const double length = getTimelineLength();
    if (length > 0.0)
    {
        const double position = getPosition ? getPosition() : transportSource.getCurrentPosition();
        const auto x = (float) (position / length * getWidth());
        g.setColour (juce::Colours::white);
        g.drawLine (x, 0.0f, x, (float) getHeight(), 2.0f);
    }
}

void WaveformView::mouseDown (const juce::MouseEvent& event)
{
    seekToX (event.x);
}

void WaveformView::mouseDrag (const juce::MouseEvent& event)
{
    seekToX (event.x);
}

void WaveformView::seekToX (int x)
{
    const double length = getTimelineLength();
    if (length <= 0.0 || getWidth() <= 0)
        return;

    // Convertit une position en pixels en une position en secondes.
    const double proportion = juce::jlimit (0.0, 1.0, (double) x / getWidth());

    if (onSeek)
        onSeek (proportion * length);
    else
        transportSource.setPosition (proportion * length);
    repaint();
}

void WaveformView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // La miniature a avancé dans la lecture du fichier : on redessine.
    repaint();
}

void WaveformView::timerCallback()
{
    // Redessine pour faire avancer la tête de lecture (y compris après un Stop).
    repaint();
}
