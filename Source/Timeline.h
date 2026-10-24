#pragma once

#include <JuceHeader.h>

// L'échelle de temps commune à la règle, aux pistes et à la tête de lecture :
// elle convertit des secondes en pixels, et inversement.
struct TimelineScale
{
    double lengthSeconds = 60.0; // durée visible, de 0 à lengthSeconds

    float timeToX (double seconds, int width) const   { return (float) (seconds / lengthSeconds * width); }
    double xToTime (float x, int width) const          { return width > 0 ? juce::jmax (0.0, x / (double) width * lengthSeconds) : 0.0; }
};

// Écrit un temps sous la forme "minutes:secondes", par exemple 1:05.
inline juce::String formatTime (double seconds)
{
    const int total = (int) seconds;
    return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
}

// La règle en haut de la vue Édition : graduations en secondes. Un clic déplace la lecture.
class TimelineRuler : public juce::Component
{
public:
    explicit TimelineRuler (const TimelineScale& s) : scale (s) {}

    std::function<void (double seconds)> onSeek;

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.3f));

        // On choisit l'écart entre deux graduations pour qu'elles soient espacées d'au moins 60 pixels.
        const double steps[] = { 1, 2, 5, 10, 15, 30, 60, 120, 300 };
        double step = steps[0];
        for (double s : steps)
        {
            step = s;
            if (scale.timeToX (s, getWidth()) >= 60.0f)
                break;
        }

        g.setFont (12.0f);
        for (double t = 0.0; t <= scale.lengthSeconds; t += step)
        {
            const float x = scale.timeToX (t, getWidth());
            g.setColour (juce::Colours::grey);
            g.drawVerticalLine ((int) x, (float) getHeight() - 8.0f, (float) getHeight());
            g.setColour (juce::Colours::lightgrey);
            g.drawText (formatTime (t), (int) x + 3, 0, 50, getHeight() - 6, juce::Justification::centredLeft);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override { seek (e.x); }
    void mouseDrag (const juce::MouseEvent& e) override { seek (e.x); }

private:
    void seek (int x)
    {
        if (onSeek)
            onSeek (scale.xToTime ((float) x, getWidth()));
    }

    const TimelineScale& scale;
};

// La tête de lecture : une ligne blanche posée PAR-DESSUS toutes les pistes.
// Elle laisse passer les clics (setInterceptsMouseClicks) pour qu'on puisse toujours attraper les clips.
class PlayheadOverlay : public juce::Component
{
public:
    explicit PlayheadOverlay (const TimelineScale& s) : scale (s)
    {
        setInterceptsMouseClicks (false, false);
    }

    std::function<double()> getPosition;

    void paint (juce::Graphics& g) override
    {
        if (getPosition == nullptr)
            return;

        const float x = scale.timeToX (getPosition(), getWidth());
        g.setColour (juce::Colours::white);
        g.drawLine (x, 0.0f, x, (float) getHeight(), 2.0f);
    }

private:
    const TimelineScale& scale;
};
