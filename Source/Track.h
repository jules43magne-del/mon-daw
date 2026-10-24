#pragma once

#include <JuceHeader.h>
#include "Clip.h"

// Une piste : des clips posés sur la ligne de temps, et des réglages de mixage.
// C'est le "modèle" : il ne dessine rien. Les vues (TrackRow, MixerStrip) l'affichent et le modifient.
class Track
{
public:
    explicit Track (juce::AudioFormatManager& formatManager);

    // Ajoute un clip à partir d'un fichier. Renvoie le clip, ou nullptr si le fichier est illisible.
    Clip* addClip (const juce::File& file, double startSeconds);
    void removeClip (Clip* clip);
    void moveClip (Clip* clip, double newStartSeconds);

    // Retire tous les clips.
    void clear();

    const std::vector<std::unique_ptr<Clip>>& getClips() const { return clips; }
    bool hasClips() const { return ! clips.empty(); }

    // Moment où se termine le dernier clip (en secondes).
    double getEndSeconds() const;

    // Fréquence de la carte son. Si elle change, les clips sont rechargés à la bonne fréquence.
    void setSampleRate (double newSampleRate);
    double getSampleRate() const { return sampleRate; }

    // Appelée par le thread audio : ajoute dans buffer les clips qui jouent entre
    // position et position + numSamples (en échantillons depuis le début de la ligne de temps).
    void render (juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 position);

    // Réglages lus par le thread audio : std::atomic pour pouvoir les changer depuis l'interface sans risque.
    std::atomic<float> gain { 0.8f };   // 0 = silence, 1 = volume d'origine
    std::atomic<float> pan { 0.0f };    // -1 = gauche, 0 = centre, +1 = droite
    std::atomic<bool> mute { false };
    std::atomic<bool> solo { false };

private:
    juce::AudioFormatManager& formatManager;
    double sampleRate = 48000.0;
    std::vector<std::unique_ptr<Clip>> clips;

    // Verrou qui empêche le thread audio de lire la liste de clips pendant que l'interface la modifie.
    // C'est un SpinLock : très léger, et le thread audio ne fait qu'*essayer* de le prendre (il n'attend jamais).
    juce::SpinLock clipLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};
