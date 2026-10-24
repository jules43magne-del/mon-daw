#include "Track.h"

Track::Track (juce::AudioFormatManager& fm)
    : formatManager (fm)
{
}

Clip* Track::addClip (const juce::File& file, double startSeconds)
{
    // Le chargement (lent) se fait AVANT de prendre le verrou : le thread audio n'est bloqué qu'un instant.
    auto clip = Clip::load (formatManager, file, juce::jmax (0.0, startSeconds), sampleRate);
    if (clip == nullptr)
        return nullptr;

    auto* result = clip.get();
    const juce::SpinLock::ScopedLockType lock (clipLock);
    clips.push_back (std::move (clip));
    return result;
}

void Track::removeClip (Clip* clip)
{
    std::unique_ptr<Clip> removed; // détruit APRÈS avoir rendu le verrou, pour aller vite

    {
        const juce::SpinLock::ScopedLockType lock (clipLock);
        for (auto it = clips.begin(); it != clips.end(); ++it)
        {
            if (it->get() == clip)
            {
                removed = std::move (*it);
                clips.erase (it);
                break;
            }
        }
    }
}

void Track::moveClip (Clip* clip, double newStartSeconds)
{
    const juce::SpinLock::ScopedLockType lock (clipLock);
    clip->startSeconds = juce::jmax (0.0, newStartSeconds);
}

void Track::clear()
{
    std::vector<std::unique_ptr<Clip>> removed;

    {
        const juce::SpinLock::ScopedLockType lock (clipLock);
        removed.swap (clips);
    }
}

double Track::getEndSeconds() const
{
    double end = 0.0;
    for (auto& clip : clips)
        end = juce::jmax (end, clip->getEndSeconds (sampleRate));
    return end;
}

void Track::setSampleRate (double newSampleRate)
{
    if (std::abs (newSampleRate - sampleRate) < 1.0)
        return;

    // On recharge chaque clip à la nouvelle fréquence, au même endroit.
    std::vector<std::unique_ptr<Clip>> reloaded;
    for (auto& clip : clips)
        if (auto newClip = Clip::load (formatManager, clip->file, clip->startSeconds, newSampleRate))
            reloaded.push_back (std::move (newClip));

    const juce::SpinLock::ScopedLockType lock (clipLock);
    sampleRate = newSampleRate;
    clips.swap (reloaded);
}

void Track::render (juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 position)
{
    // Si l'interface est en train de modifier les clips, on ne l'attend pas : ce bloc sera silencieux.
    const juce::SpinLock::ScopedTryLockType lock (clipLock);
    if (! lock.isLocked())
        return;

    const juce::int64 blockEnd = position + numSamples;

    for (auto& clip : clips)
    {
        const auto clipStart = (juce::int64) (clip->startSeconds * sampleRate);
        const auto clipEnd = clipStart + clip->audio.getNumSamples();

        // Le clip ne joue pas pendant ce bloc ? On passe au suivant.
        if (clipEnd <= position || clipStart >= blockEnd)
            continue;

        // La partie commune entre le bloc et le clip :
        const auto from = juce::jmax (position, clipStart);
        const auto to = juce::jmin (blockEnd, clipEnd);

        const int destOffset = (int) (from - position);   // où écrire dans le bloc
        const int srcOffset = (int) (from - clipStart);   // où lire dans le clip
        const int count = (int) (to - from);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom (ch, destOffset, clip->audio, juce::jmin (ch, clip->audio.getNumChannels() - 1), srcOffset, count);
    }
}
