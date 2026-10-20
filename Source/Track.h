#pragma once

#include <JuceHeader.h>

// Une piste : un fichier audio et ses réglages de mixage.
// C'est le "modèle" : il ne dessine rien. Les vues (TrackRow, MixerStrip) l'affichent et le modifient.
class Track
{
public:
    explicit Track (juce::AudioFormatManager& formatManager);

    // Charge un fichier dans la piste. Renvoie false si le fichier n'est pas lisible.
    bool loadFile (const juce::File& file);

    bool hasFile() const                  { return readerSource != nullptr; }
    juce::File getFile() const            { return loadedFile; }
    double getLengthInSeconds() const     { return lengthInSeconds; }

    // Déplace la lecture de cette piste (en secondes).
    void seek (double seconds);

    juce::AudioTransportSource& getTransport() { return transport; }

    // Réglages lus par le thread audio : std::atomic pour pouvoir les changer depuis l'interface sans risque.
    std::atomic<float> gain { 0.8f };   // 0 = silence, 1 = volume d'origine
    std::atomic<float> pan { 0.0f };    // -1 = gauche, 0 = centre, +1 = droite
    std::atomic<bool> mute { false };
    std::atomic<bool> solo { false };

private:
    juce::AudioFormatManager& formatManager;

    // L'ordre compte : transport est détruit avant readerSource, qu'il utilise.
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;

    juce::File loadedFile;
    double lengthInSeconds = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};
