#include "MainComponent.h"

MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = tracks.add (new Track (formatManager));

        // Ligne de la vue Édition
        auto* row = rows.add (new TrackRow (i + 1, *track, timelineScale, formatManager, thumbnailCache));
        row->onClipsChanged = [this] { updateTimeline(); updateStripNames(); };
        row->getPlayheadPosition = [this] { return getPosition(); };
        row->getLane().onSeek = [this] (double seconds) { seekAll (seconds); };
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

    // Règle et tête de lecture. La tête de lecture est ajoutée en dernier pour être dessinée par-dessus les pistes.
    ruler.onSeek = [this] (double seconds) { seekAll (seconds); };
    addAndMakeVisible (ruler);
    playhead.getPosition = [this] { return getPosition(); };
    addAndMakeVisible (playhead);

    // Barre de transport
    playButton.onClick  = [this] { play(); };
    pauseButton.onClick = [this] { pause(); };
    stopButton.onClick  = [this] { stop(); };
    addAndMakeVisible (playButton);
    addAndMakeVisible (pauseButton);
    addAndMakeVisible (stopButton);

    openSessionButton.onClick = [this] { chooseSessionToOpen(); };
    saveSessionButton.onClick = [this] { chooseSessionToSave(); };
    addAndMakeVisible (openSessionButton);
    addAndMakeVisible (saveSessionButton);

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
    // Si on est arrivé après le dernier clip, on repart du début.
    if (getPosition() >= getSongEnd())
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
    playPosition = (juce::int64) (juce::jmax (0.0, seconds) * deviceSampleRate.load());
    playhead.repaint();
}

double MainComponent::getPosition() const
{
    return (double) playPosition.load() / deviceSampleRate.load();
}

double MainComponent::getSongEnd() const
{
    // La fin du morceau, c'est la fin du dernier clip, toutes pistes confondues.
    double end = 0.0;
    for (auto* track : tracks)
        end = juce::jmax (end, track->getEndSeconds());
    return end;
}

bool MainComponent::anyTrackLoaded() const
{
    for (auto* track : tracks)
        if (track->hasClips())
            return true;
    return false;
}

void MainComponent::updateTimeline()
{
    // La ligne de temps montre au moins 1 minute, et toujours 10 secondes de marge après le dernier clip,
    // pour avoir de la place où glisser les clips. On arrondit à la dizaine de secondes.
    const double wanted = std::ceil ((getSongEnd() + 10.0) / 10.0) * 10.0;
    timelineScale.lengthSeconds = juce::jmax (60.0, wanted);

    for (auto* row : rows)
        row->refresh();

    ruler.repaint();
    playhead.repaint();
}

void MainComponent::updateStripNames()
{
    // Chaque tranche prend le nom du premier clip de sa piste.
    for (int i = 0; i < tracks.size(); ++i)
    {
        const auto& clips = tracks[i]->getClips();
        strips[i]->setName (clips.empty() ? "Piste " + juce::String (i + 1)
                                          : clips.front()->file.getFileNameWithoutExtension());
    }
}

void MainComponent::timerCallback()
{
    // Si la carte son a changé de fréquence, on recharge les clips à la bonne fréquence.
    for (auto* track : tracks)
    {
        if (std::abs (track->getSampleRate() - deviceSampleRate.load()) >= 1.0)
        {
            track->setSampleRate (deviceSampleRate.load());
            for (auto* row : rows)
                row->getLane().rebuild();
        }
    }

    // Fin du morceau : la lecture a dépassé le dernier clip.
    if (playing && getPosition() >= getSongEnd())
        playing = false;

    playhead.repaint();

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
// Sessions
//
// Une session est un petit fichier texte au format XML, par exemple :
//
//   <MonDAWSession version="2">
//     <Master gain="0.80"/>
//     <Track number="1" gain="0.80" pan="0.00" mute="0" solo="0">
//       <Clip file="C:\Users\...\batterie.wav" relativeFile="batterie.wav" start="0.00"/>
//       <Clip file="C:\Users\...\batterie.wav" relativeFile="batterie.wav" start="8.00"/>
//     </Track>
//     ...
//   </MonDAWSession>
//
// Les fichiers audio ne sont pas copiés : la session retient seulement où ils se trouvent.

void MainComponent::chooseSessionToSave()
{
    sessionChooser = std::make_unique<juce::FileChooser> ("Enregistrer la session",
                                                          currentSessionFile,
                                                          "*.mondaw");

    auto chooserFlags = juce::FileBrowserComponent::saveMode
               | juce::FileBrowserComponent::canSelectFiles
               | juce::FileBrowserComponent::warnAboutOverwriting;

    sessionChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{})
            return; // l'utilisateur a annulé

        if (! saveSession (file.withFileExtension ("mondaw")))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Erreur", "Impossible d'enregistrer la session.");
    });
}

void MainComponent::chooseSessionToOpen()
{
    sessionChooser = std::make_unique<juce::FileChooser> ("Ouvrir une session",
                                                          currentSessionFile,
                                                          "*.mondaw");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    sessionChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{})
            return; // l'utilisateur a annulé

        if (! openSession (file))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Erreur", "Ce fichier n'est pas une session MonDAW valide.");
    });
}

bool MainComponent::saveSession (const juce::File& sessionFile)
{
    juce::XmlElement session ("MonDAWSession");
    session.setAttribute ("version", 2);

    session.createNewChildElement ("Master")->setAttribute ("gain", juce::String (masterGain.load(), 2));

    for (int i = 0; i < tracks.size(); ++i)
    {
        auto* track = tracks[i];
        auto* xml = session.createNewChildElement ("Track");
        xml->setAttribute ("number", i + 1);

        xml->setAttribute ("gain", juce::String (track->gain.load(), 2)); // 2 chiffres après la virgule suffisent
        xml->setAttribute ("pan", juce::String (track->pan.load(), 2));
        xml->setAttribute ("mute", track->mute.load());
        xml->setAttribute ("solo", track->solo.load());

        for (auto& clip : track->getClips())
        {
            // On garde le chemin complet, et aussi le chemin depuis le dossier de la session :
            // si tu déplaces le dossier entier (session + sons), la session les retrouvera quand même.
            auto* clipXml = xml->createNewChildElement ("Clip");
            clipXml->setAttribute ("file", clip->file.getFullPathName());
            clipXml->setAttribute ("relativeFile", clip->file.getRelativePathFrom (sessionFile.getParentDirectory()));
            clipXml->setAttribute ("start", juce::String (clip->startSeconds, 2));
        }
    }

    if (! session.writeTo (sessionFile))
        return false;

    currentSessionFile = sessionFile;
    return true;
}

bool MainComponent::openSession (const juce::File& sessionFile)
{
    auto session = juce::XmlDocument::parse (sessionFile);
    if (session == nullptr || ! session->hasTagName ("MonDAWSession"))
        return false;

    stop();

    if (auto* master = session->getChildByName ("Master"))
        masterStrip.setGain ((float) master->getDoubleAttribute ("gain", 0.8));

    juce::StringArray missingFiles;

    for (int i = 0; i < tracks.size(); ++i)
    {
        // On cherche la ligne <Track number="i+1">. Si elle n'existe pas, la piste reste vide.
        auto* xml = session->getChildByAttribute ("number", juce::String (i + 1));

        rows[i]->clear();

        if (xml == nullptr)
            continue;

        // Retrouve un son : d'abord à côté de la session, sinon à l'emplacement d'origine.
        auto loadClip = [&] (const juce::XmlElement& clipXml)
        {
            auto file = sessionFile.getParentDirectory().getChildFile (clipXml.getStringAttribute ("relativeFile"));
            if (! file.existsAsFile())
                file = juce::File (clipXml.getStringAttribute ("file"));

            if (! rows[i]->addClip (file, clipXml.getDoubleAttribute ("start", 0.0)))
                missingFiles.addIfNotAlreadyThere (clipXml.getStringAttribute ("file"));
        };

        // Version 2 : une balise <Clip> par clip.
        for (auto* clipXml : xml->getChildWithTagNameIterator ("Clip"))
            loadClip (*clipXml);

        // Version 1 (étape 5) : un seul fichier par piste, qui commence au début.
        if (xml->hasAttribute ("file"))
            loadClip (*xml);

        strips[i]->setGain ((float) xml->getDoubleAttribute ("gain", 0.8));
        strips[i]->setPan ((float) xml->getDoubleAttribute ("pan", 0.0));
        strips[i]->setMute (xml->getBoolAttribute ("mute", false));
        strips[i]->setSolo (xml->getBoolAttribute ("solo", false));
    }

    updateTimeline();
    updateStripNames();
    seekAll (0.0);
    currentSessionFile = sessionFile;

    if (! missingFiles.isEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "Fichiers introuvables",
                                                juce::String (juce::CharPointer_UTF8 ("Ces fichiers n'ont pas \xc3\xa9t\xc3\xa9 retrouv\xc3\xa9s :\n")) + missingFiles.joinIntoString ("\n"));

    return true;
}

//==============================================================================
// Audio

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // Les clips seront convertis à cette fréquence (dans timerCallback, sur le thread de l'interface).
    deviceSampleRate = sampleRate;

    // On réserve la mémoire maintenant, pas dans getNextAudioBlock (règle d'or du temps réel).
    trackBuffer.setSize (2, samplesPerBlockExpected * 2);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    // En pause ou à l'arrêt : silence, et la position n'avance pas.
    if (! playing)
        return;

    // La position au début de ce bloc. Toutes les pistes lisent leurs clips à partir d'ici.
    juce::int64 position = playPosition.load();

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

        // 1. On récupère le son des clips de la piste qui jouent pendant ce bloc.
        trackBuffer.clear();
        track->render (trackBuffer, numSamples, position);

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

    // On avance la position... sauf si l'interface l'a changée pendant ce bloc (clic sur la règle) :
    // compare_exchange ne remplace la valeur que si elle vaut toujours "position".
    playPosition.compare_exchange_strong (position, position + numSamples);
}

void MainComponent::releaseResources()
{
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

    // Sessions à droite, le compteur entre les deux.
    saveSessionButton.setBounds (transportBar.removeFromRight (120));
    transportBar.removeFromRight (8);
    openSessionButton.setBounds (transportBar.removeFromRight (120));
    transportBar.removeFromRight (16);
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

    // Vue Édition : la règle en haut, puis les pistes se partagent la hauteur restante.
    // La règle et la tête de lecture sont alignées sur les voies des pistes (après la colonne de gauche).
    const int laneLeft = TrackRow::padding + TrackRow::headerWidth + TrackRow::padding;
    auto rulerArea = area.removeFromTop (24);
    ruler.setBounds (rulerArea.withTrimmedLeft (laneLeft).withTrimmedRight (TrackRow::padding));

    const auto editArea = area;
    const int rowHeight = area.getHeight() / numTracks;
    for (auto* row : rows)
        row->setBounds (area.removeFromTop (rowHeight).reduced (0, 3));

    playhead.setBounds (rulerArea.getUnion (editArea).withTrimmedLeft (laneLeft).withTrimmedRight (TrackRow::padding));

    for (auto* row : rows)
        row->refresh();
}
