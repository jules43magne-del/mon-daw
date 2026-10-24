#pragma once

#include <JuceHeader.h>

// Un clip : un fichier audio posé sur la ligne de temps d'une piste, à partir d'un certain moment.
// Le son est chargé entièrement en mémoire, déjà converti à la fréquence de la carte son :
// le thread audio n'a plus qu'à le recopier, sans lire le disque.
struct Clip
{
    juce::File file;
    double startSeconds = 0.0;      // où le clip commence sur la ligne de temps
    juce::AudioBuffer<float> audio; // les échantillons, à la fréquence de la carte son

    double getLengthSeconds (double sampleRate) const { return audio.getNumSamples() / sampleRate; }
    double getEndSeconds (double sampleRate) const    { return startSeconds + getLengthSeconds (sampleRate); }

    // Lit un fichier et le convertit à la fréquence voulue. Renvoie nullptr si le fichier est illisible.
    static std::unique_ptr<Clip> load (juce::AudioFormatManager& formatManager,
                                       const juce::File& file,
                                       double startSeconds,
                                       double sampleRate);
};
