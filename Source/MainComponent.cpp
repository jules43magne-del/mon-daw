#include "MainComponent.h"

MainComponent::MainComponent()
{
    // Boutons
    addAndMakeVisible (openButton);
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible (playButton);
    playButton.onClick = [this] { transportSource.start(); };

    addAndMakeVisible (pauseButton);
    pauseButton.onClick = [this] { transportSource.stop(); }; // s'arrête sans revenir au début

    addAndMakeVisible (stopButton);
    stopButton.onClick = [this] { transportSource.stop(); transportSource.setPosition (0.0); };

    // Fader de volume, vertical comme sur une console de mixage
    volumeSlider.setSliderStyle (juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
    volumeSlider.setRange (0.0, 1.0, 0.01);
    volumeSlider.setValue (0.8);
    volumeSlider.onValueChange = [this] { transportSource.setGain ((float) volumeSlider.getValue()); };
    addAndMakeVisible (volumeSlider);

    volumeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (volumeLabel);
    addAndMakeVisible (fileLabel);
    addAndMakeVisible (waveformView);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener (this);
    transportSource.setGain ((float) volumeSlider.getValue());
    updateButtons();

    setSize (700, 460);

    // 0 entrée, 2 sorties (stéréo)
    setAudioChannels (0, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Sans fichier, on envoie du silence.
    if (readerSource == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock (bufferToFill);
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (16);

    // Bande du bas : la forme d'onde
    waveformView.setBounds (area.removeFromBottom (140));
    area.removeFromBottom (16);

    // Colonne de droite : le fader
    auto faderArea = area.removeFromRight (90);
    volumeLabel.setBounds (faderArea.removeFromTop (24));
    volumeSlider.setBounds (faderArea);

    area.removeFromRight (16);

    // Colonne de gauche : les boutons empilés
    openButton.setBounds (area.removeFromTop (40));
    area.removeFromTop (10);
    fileLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (10);
    playButton.setBounds (area.removeFromTop (40));
    area.removeFromTop (10);
    pauseButton.setBounds (area.removeFromTop (40));
    area.removeFromTop (10);
    stopButton.setBounds (area.removeFromTop (40));
}

void MainComponent::openButtonClicked()
{
    chooser = std::make_unique<juce::FileChooser> ("Choisis un fichier audio",
                                                   juce::File{},
                                                   "*.wav;*.aif;*.aiff");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{})
            return; // l'utilisateur a annulé

        if (auto* reader = formatManager.createReaderFor (file))
        {
            auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
            transportSource.stop();
            transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
            readerSource = std::move (newSource);
            fileLabel.setText (file.getFileName(), juce::dontSendNotification);
            waveformView.setFile (file);
            updateButtons();
        }
    });
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // Appelé quand la lecture démarre ou s'arrête (y compris en fin de fichier).
    if (source == &transportSource)
    {
        if (! transportSource.isPlaying() && transportSource.hasStreamFinished())
            transportSource.setPosition (0.0);

        updateButtons();
    }
}

void MainComponent::updateButtons()
{
    const bool hasFile = readerSource != nullptr;
    const bool playing = transportSource.isPlaying();
    playButton.setEnabled (hasFile && ! playing);
    pauseButton.setEnabled (hasFile && playing);
    stopButton.setEnabled (hasFile);
}
