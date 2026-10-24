#include "Clip.h"

std::unique_ptr<Clip> Clip::load (juce::AudioFormatManager& formatManager,
                                  const juce::File& file,
                                  double startSeconds,
                                  double sampleRate)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return nullptr;

    // 1. On lit tout le fichier (2 canaux au maximum : un fichier mono est recopié à gauche et à droite).
    const int numChannels = 2;
    const int fileLength = (int) reader->lengthInSamples;
    juce::AudioBuffer<float> original (numChannels, fileLength);
    reader->read (&original, 0, fileLength, 0, true, true);

    auto clip = std::make_unique<Clip>();
    clip->file = file;
    clip->startSeconds = startSeconds;

    // 2. Si le fichier n'a pas la même fréquence que la carte son (44100 Hz contre 48000 Hz par exemple),
    //    on le "rééchantillonne", sinon il jouerait trop vite ou trop lentement.
    if (std::abs (reader->sampleRate - sampleRate) < 1.0)
    {
        clip->audio = std::move (original);
    }
    else
    {
        const double ratio = reader->sampleRate / sampleRate;
        const int newLength = (int) (fileLength / ratio);
        clip->audio.setSize (numChannels, newLength);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.process (ratio, original.getReadPointer (ch), clip->audio.getWritePointer (ch), newLength);
        }
    }

    return clip;
}
