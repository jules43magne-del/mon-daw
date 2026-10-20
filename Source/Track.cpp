#include "Track.h"

Track::Track (juce::AudioFormatManager& fm)
    : formatManager (fm)
{
}

bool Track::loadFile (const juce::File& file)
{
    auto* reader = formatManager.createReaderFor (file);
    if (reader == nullptr)
        return false;

    const double sampleRate = reader->sampleRate;

    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
    transport.setSource (newSource.get(), 0, nullptr, sampleRate);
    readerSource = std::move (newSource);

    loadedFile = file;
    lengthInSeconds = (double) reader->lengthInSamples / sampleRate;
    return true;
}

void Track::clear()
{
    transport.setSource (nullptr);
    readerSource.reset();
    loadedFile = juce::File();
    lengthInSeconds = 0.0;
}

void Track::seek (double seconds)
{
    transport.setPosition (seconds);

    // Le transport de chaque piste reste toujours "en marche" : c'est le mixeur qui décide
    // s'il avance ou non. Ainsi toutes les pistes restent synchronisées à l'échantillon près.
    if (hasFile())
        transport.start();
}
