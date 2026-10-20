#include "TrackRow.h"

TrackRow::TrackRow (int number, Track& t, juce::AudioFormatManager& formatManager)
    : trackNumber (number),
      track (t),
      waveform (formatManager, t.getTransport())
{
    numberLabel.setText ("Piste " + juce::String (trackNumber), juce::dontSendNotification);
    numberLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (numberLabel);

    loadButton.onClick = [this] { chooseFile(); };
    addAndMakeVisible (loadButton);

    fileLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    fileLabel.setMinimumHorizontalScale (0.5f);
    addAndMakeVisible (fileLabel);

    addAndMakeVisible (waveform);
}

void TrackRow::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Choisis un fichier audio pour la piste " + juce::String (trackNumber),
                                                   juce::File{},
                                                   "*.wav;*.aif;*.aiff");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{})
            return; // l'utilisateur a annulé

        if (track.loadFile (file))
        {
            fileLabel.setText (file.getFileName(), juce::dontSendNotification);
            fileLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            waveform.setFile (file);

            if (onFileLoaded)
                onFileLoaded();
        }
    });
}

void TrackRow::paint (juce::Graphics& g)
{
    g.setColour (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).brighter (0.1f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
}

void TrackRow::resized()
{
    auto area = getLocalBounds().reduced (6);

    // Colonne de gauche : numéro, bouton, nom du fichier
    auto header = area.removeFromLeft (150);
    numberLabel.setBounds (header.removeFromTop (22));
    loadButton.setBounds (header.removeFromTop (26).reduced (2, 0));
    fileLabel.setBounds (header);

    area.removeFromLeft (6);
    waveform.setBounds (area);
}
