#include "MainComponent.h"

MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = tracks.add (new Track (formatManager));

        // Ligne de la vue Édition
        auto* row = rows.add (new TrackRow (i + 1, *track, formatManager));
        row->onFileLoaded = [this, i]
        {
            strips[i]->setName (tracks[i]->getFile().getFileNameWithoutExtension());
            updateTimeline();
            seekAll (getPosition()); // la nouvelle piste se cale sur les autres
        };
        row->getWaveform().onSeek = [this] (double seconds) { seekAll (seconds); };
        row->getWaveform().getPosition = [this] { return getPosition(); };
        addAndMakeVisible (row);

        // Tranche de la console : chaque réglage met à jour la piste correspondante.
        auto* strip = strips.add (new MixerStrip ("Piste " + juce::String (i + 1), false));
        strip->onGainChange = [track] (float value) { track->gain = value; };
        strip->onPanChange  = [track] (float value) { track->pan = value; };
        strip->onMuteChange = [track] (bool value)  { track->mute = value; };
        strip->onSoloChange = [track] (bool value)  { track->solo = value; };
        addAndMakeVisible (strip);
    }

    masterStrip.onGainChange = [this] (float value) { masterGain = value; };
    addAndMakeVisible (masterStrip);

    // Barre de transport
    playButton.onClick  = [this] { play(); };
    pauseButton.onClick = [this] { pause(); };
    stopButton.onClick  = [this] { stop(); };
    addAndMakeVisible (playButton);
    addAndMakeVisible (pauseButton);
    addAndMakeVisible (stopButton);

    timeLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    timeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (timeLabel);

    startTimerHz (15);
    timerCallback();

    setSize (1000, 760);

    // 0 entrée, 2 sorties (stéréo)
    setAudioChannels (0, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
// Transport

void MainComponent::play()
{
    // Si toutes les pistes sont arrivées au bout, on repart du début.
    bool allFinished = true;
    for (auto* track : tracks)
        if (track->hasFile() && ! track->getTransport().hasStreamFinished())
            allFinished = false;

    if (allFinished)
        seekAll (0.0);

    playing = true;
}

void MainComponent::pause()
{
    playing = false; // le mixeur arrête de faire avancer les pistes, la position est conservée
}

void MainComponent::stop()
{
    playing = false;
    seekAll (0.0);
}

void MainComponent::seekAll (double seconds)
{
    for (auto* track : tracks)
        track->seek (seconds);
}

double MainComponent::getPosition() const
{
    // Toutes les pistes avancent ensemble : on prend la plus avancée
    // (une piste plus courte s'arrête à sa fin).
    double position = 0.0;
    for (auto* track : tracks)
        if (track->hasFile())
            position = juce::jmax (position, track->getTransport().getCurrentPosition());
    return position;
}

bool MainComponent::anyTrackLoaded() const
{
    for (auto* track : tracks)
        if (track->hasFile())
            return true;
    return false;
}

void MainComponent::updateTimeline()
{
    // Toutes les formes d'onde partagent la même échelle de temps : celle de la piste la plus longue.
    double longest = 0.0;
    for (auto* track : tracks)
        longest = juce::jmax (longest, track->getLengthInSeconds());

    for (auto* row : rows)
        row->getWaveform().setTimelineLength (longest);
}

void MainComponent::timerCallback()
{
    // Fin du morceau : plus aucune piste ne joue.
    if (playing)
    {
        bool anyRunning = false;
        for (auto* track : tracks)
            if (track->hasFile() && track->getTransport().isPlaying())
                anyRunning = true;

        if (! anyRunning)
            playing = false;
    }

    // Compteur "minutes:secondes.dixièmes"
    const double position = getPosition();
    const int minutes = (int) position / 60;
    const double seconds = position - minutes * 60;
    timeLabel.setText (juce::String (minutes) + ":" + juce::String (seconds, 1).paddedLeft ('0', 4),
                       juce::dontSendNotification);

    const bool loaded = anyTrackLoaded();
    playButton.setEnabled (loaded && ! playing);
    pauseButton.setEnabled (playing);
    stopButton.setEnabled (loaded);
}

//==============================================================================
// Audio

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    for (auto* track : tracks)
        track->getTransport().prepareToPlay (samplesPerBlockExpected, sampleRate);

    // On réserve la mémoire maintenant, pas dans getNextAudioBlock (règle d'or du temps réel).
    trackBuffer.setSize (2, samplesPerBlockExpected * 2);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    // En pause ou à l'arrêt : silence, et les pistes n'avancent pas.
    if (! playing)
        return;

    auto& output = *bufferToFill.buffer;
    const int numSamples = bufferToFill.numSamples;
    const int numOutputChannels = juce::jmin (2, output.getNumChannels());

    // Garde-fou : si la carte son envoie un bloc plus grand que prévu, on agrandit le brouillon.
    trackBuffer.setSize (2, numSamples, false, false, true);

    // Si au moins une piste est en solo, seules les pistes en solo s'entendent.
    bool anySolo = false;
    for (auto* track : tracks)
        if (track->solo)
            anySolo = true;

    for (int i = 0; i < tracks.size(); ++i)
    {
        auto* track = tracks[i];

        // 1. On lit le bloc de la piste (même muette, pour qu'elle avance avec les autres).
        juce::AudioSourceChannelInfo trackInfo (&trackBuffer, 0, numSamples);
        track->getTransport().getNextAudioBlock (trackInfo);

        // 2. Volume et panoramique "à puissance constante" : le son garde la même force au centre et sur les côtés.
        const bool audible = ! track->mute && (! anySolo || track->solo);
        const float gain = audible ? track->gain.load() : 0.0f;
        const float angle = (track->pan.load() + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
        trackBuffer.applyGain (0, 0, numSamples, gain * std::cos (angle));
        trackBuffer.applyGain (1, 0, numSamples, gain * std::sin (angle));

        // 3. Vumètre de la piste
        auto& meter = strips[i]->getMeter();
        meter.setLevel (0, trackBuffer.getMagnitude (0, 0, numSamples));
        meter.setLevel (1, trackBuffer.getMagnitude (1, 0, numSamples));

        // 4. On ajoute la piste au mixage (le "bus" Master).
        for (int ch = 0; ch < numOutputChannels; ++ch)
            output.addFrom (ch, bufferToFill.startSample, trackBuffer, ch, 0, numSamples);
    }

    // Master : volume général et vumètre.
    output.applyGain (bufferToFill.startSample, numSamples, masterGain.load());
    for (int ch = 0; ch < numOutputChannels; ++ch)
        masterStrip.getMeter().setLevel (ch, output.getMagnitude (ch, bufferToFill.startSample, numSamples));
}

void MainComponent::releaseResources()
{
    for (auto* track : tracks)
        track->getTransport().releaseResources();
}

//==============================================================================
// Interface

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (12);

    // Barre de transport
    auto transportBar = area.removeFromTop (40);
    playButton.setBounds (transportBar.removeFromLeft (110));
    transportBar.removeFromLeft (8);
    pauseButton.setBounds (transportBar.removeFromLeft (110));
    transportBar.removeFromLeft (8);
    stopButton.setBounds (transportBar.removeFromLeft (110));
    timeLabel.setBounds (transportBar);

    area.removeFromTop (12);

    // Console de mixage en bas : une tranche par piste, le Master à droite.
    auto mixerArea = area.removeFromBottom (300);
    masterStrip.setBounds (mixerArea.removeFromRight (110));
    mixerArea.removeFromRight (16);
    for (auto* strip : strips)
    {
        strip->setBounds (mixerArea.removeFromLeft (110));
        mixerArea.removeFromLeft (8);
    }

    area.removeFromBottom (12);

    // Vue Édition : les pistes se partagent la hauteur restante.
    const int rowHeight = area.getHeight() / numTracks;
    for (auto* row : rows)
        row->setBounds (area.removeFromTop (rowHeight).reduced (0, 3));
}
