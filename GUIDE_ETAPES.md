# Guide des étapes de MonDAW

Ce guide raconte comment MonDAW s'est construit, étape par étape. Pour chaque étape, il donne :
**ce que ça fait**, **les fichiers concernés**, **comment ça marche**, **ce que tu apprends** et
**des exercices** pour t'approprier le code.

Lis-le avec le code ouvert à côté dans Visual Studio. Pour les bases du langage (variables, classes,
pointeurs...), garde aussi [BASES_CPP.md](BASES_CPP.md) sous la main.

---

## Vue d'ensemble : comment le programme est organisé

```
Main.cpp                 crée la fenêtre
└── MainComponent        la fenêtre + le moteur de mixage
    ├── Track  ×4        le "modèle" d'une piste : fichier + réglages (pas d'affichage)
    ├── TrackRow ×4      vue Édition : bouton Charger, nom, forme d'onde
    │   └── WaveformView    dessine la forme d'onde et la tête de lecture
    ├── MixerStrip ×4    vue Mix : Mute, Solo, panoramique, fader, vumètre
    │   └── LevelMeter      dessine le vumètre
    └── MixerStrip       la tranche Master
```

Deux idées reviennent partout :

1. **Le modèle et les vues sont séparés.** `Track` contient les données (le son, le volume...).
   `TrackRow` et `MixerStrip` les affichent et les modifient. C'est l'organisation de tous les
   logiciels un peu gros : on peut ajouter une nouvelle vue sans toucher au son.
2. **Il y a deux "fils d'exécution" (threads).** Le *thread de l'interface* gère les clics et le
   dessin. Le *thread audio* fabrique le son, des centaines de fois par seconde, et ne doit jamais
   attendre. Ils se parlent à travers des variables `std::atomic` (voir l'étape 3).

---

## Étape 1 : un lecteur de fichier WAV

**Ce que ça fait** : une fenêtre avec « Ouvrir », « Lecture », « Stop » et un fader de volume.

**Fichiers** : `CMakeLists.txt`, `Source/Main.cpp`, `Source/MainComponent.h/.cpp`.

**Comment ça marche**

- `CMakeLists.txt` est la recette de construction. `FetchContent` télécharge JUCE tout seul, et
  `juce_add_gui_app` déclare une application avec fenêtre.
- `Main.cpp` contient la classe `MonDAWApplication`. JUCE l'appelle au démarrage (`initialise`)
  et elle crée une `MainWindow`, qui affiche un `MainComponent`.
- `MainComponent` hérite de `juce::AudioAppComponent`. Grâce à cet héritage, JUCE ouvre la carte
  son et appelle trois fonctions pour nous :
  - `prepareToPlay` une fois, avant de démarrer : « prépare-toi, voici la fréquence d'échantillonnage » ;
  - `getNextAudioBlock` en boucle : « donne-moi les 512 prochains échantillons » ;
  - `releaseResources` à la fin.
- Pour lire un fichier, trois objets JUCE s'emboîtent :
  `AudioFormatManager` (sait lire WAV et AIFF) → `AudioFormatReaderSource` (lit *ce* fichier) →
  `AudioTransportSource` (lecture, arrêt, position).

**Ce que tu apprends** : la structure d'une app JUCE, les classes, l'héritage, les lambdas
(`button.onClick = [this] { ... };`).

**Exercices** (déjà faits ✔) : les 4 exercices de [BASES_CPP.md](BASES_CPP.md).

---

## Étape 2 : la forme d'onde et la tête de lecture

**Ce que ça fait** : le dessin du son s'affiche, une ligne blanche suit la lecture, et un clic
dans le dessin déplace la lecture. Ton bouton Pause est arrivé ici.

**Fichiers** : `Source/WaveformView.h/.cpp` (nouveau).

**Comment ça marche**

- `juce::AudioThumbnail` lit le fichier en arrière-plan et en garde un résumé (un point pour 512
  échantillons). C'est ce résumé qu'on dessine, sinon ce serait beaucoup trop lent.
- `paint()` est appelée par JUCE chaque fois qu'il faut redessiner. On y dessine le fond noir, puis
  la forme d'onde (`drawChannels`), puis la tête de lecture (`drawLine`).
- Un `juce::Timer` appelle `timerCallback()` 30 fois par seconde. On y demande `repaint()`, ce qui
  fait avancer la tête de lecture.
- `mouseDown` et `mouseDrag` reçoivent les clics. On convertit la position en pixels en secondes
  avec une règle de trois : `secondes = x / largeur × durée`.

**Ce que tu apprends** : dessiner (`juce::Graphics`), réagir à la souris, les timers.

**Exercices**

1. Change la couleur de la forme d'onde (cherche `lightgreen` dans `WaveformView.cpp`). Les
   couleurs disponibles sont dans `juce::Colours`. Tape `juce::Colours::` et Visual Studio te les liste.
2. Rends la tête de lecture plus épaisse (le dernier nombre de `drawLine`).

---

## Étape 3 : le vumètre

**Ce que ça fait** : deux barres (gauche et droite) montrent le niveau du son, du vert au rouge.

**Fichiers** : `Source/LevelMeter.h/.cpp` (nouveau).

**Comment ça marche**

C'est la première fois que les deux threads doivent se parler, et c'est là que la règle d'or
de l'audio compte vraiment.

- **Côté audio**, dans `getNextAudioBlock`, on mesure le pic du bloc (`getMagnitude`) et on le
  dépose avec `meter.setLevel(...)`. Cette fonction ne fait qu'écrire un nombre : pas de dessin,
  pas d'attente.
- **Côté interface**, le timer du vumètre récupère ce nombre 30 fois par seconde (`exchange`),
  le fait redescendre doucement (`× 0.85`) et redessine.
- Le nombre est stocké dans un `std::atomic<float>`. Un `atomic` garantit qu'un thread ne lira
  jamais un nombre « à moitié écrit » par l'autre.
- L'échelle est en **décibels** (de -60 dB à 0 dB), comme sur une vraie console. C'est l'unité qui
  correspond à notre oreille : chaque -6 dB divise l'amplitude par deux.

**Ce que tu apprends** : les threads, `std::atomic`, les décibels.

**Exercices**

1. Fais redescendre le vumètre plus lentement : remplace `0.85f` par `0.95f`.
2. Change l'échelle pour qu'elle commence à -40 dB au lieu de -60 dB (`minDecibels`).

---

## Étape 4 : 4 pistes et la console de mixage

**Ce que ça fait** : 4 pistes jouées ensemble, une vue Édition (formes d'onde) et une console
façon Pro Tools (Mute, Solo, panoramique, fader, vumètre, Master).

**Fichiers** : `Source/Track.h/.cpp`, `Source/TrackRow.h/.cpp`, `Source/MixerStrip.h/.cpp` (nouveaux),
`Source/MainComponent.cpp` (réécrit).

**Comment ça marche**

- **`Track`** est le modèle d'une piste. Ses réglages (`gain`, `pan`, `mute`, `solo`) sont des
  `std::atomic`, puisque l'interface les écrit et que le thread audio les lit.
- **`MixerStrip`** ne connaît pas l'audio. Quand on bouge le fader, il appelle `onGainChange`, et
  c'est `MainComponent` qui a branché ce callback sur la bonne piste :
  `strip->onGainChange = [track] (float value) { track->gain = value; };`.
  On pourra donc réutiliser la même tranche pour autre chose, comme le Master.
- **Le mixeur**, c'est `MainComponent::getNextAudioBlock`. Pour chaque piste :
  1. on lit son bloc dans un tampon préparé à l'avance (`trackBuffer`) ;
  2. on applique le mute/solo, le volume et le panoramique ;
  3. on envoie son niveau à son vumètre ;
  4. on l'additionne dans la sortie (`addFrom`). **Mixer, c'est additionner.**
  Ensuite, on applique le volume du Master.
- **Le panoramique « à puissance constante »** : gauche = cos(angle), droite = sin(angle). Au centre,
  chaque côté reçoit 0,707, et le son paraît aussi fort qu'à gauche ou à droite.
- **La synchronisation** : toutes les pistes avancent dans le *même* appel de
  `getNextAudioBlock`, donc elles ne peuvent pas se décaler. Pause, c'est juste
  `playing = false` : le mixeur arrête de les faire avancer.
- **`OwnedArray`** est un tableau qui détruit tout seul les objets qu'il contient. On s'en sert pour
  les 4 pistes, les 4 lignes et les 4 tranches.

**Ce que tu apprends** : séparer modèle et vue, les callbacks (`std::function`), un vrai moteur de
mixage.

**Exercices**

1. Passe à 6 pistes. Une seule ligne suffit (`numTracks` dans `MainComponent.h`). Regarde comment
   tout le reste s'adapte.
2. Ajoute un bouton « C » (centre) dans `MixerStrip` qui remet le panoramique à 0. Inspire-toi de
   `muteButton`.

---

## Étape 5 : enregistrer et rouvrir une session

**Ce que ça fait** : les boutons « Enregistrer... » et « Ouvrir... » sauvent tout ton mixage dans un
fichier `.mondaw` : quels sons sur quelles pistes, volumes, panoramiques, mute, solo et Master.

**Fichiers** : `Source/MainComponent.cpp` (fonctions `saveSession` et `openSession`), plus de petits
ajouts dans `Track`, `TrackRow`, `MixerStrip` et `WaveformView` (`clear`, `loadFile`, `setGain`...).

**Comment ça marche**

- Le fichier de session est du **XML**, un format texte à balises. Ouvre un `.mondaw` avec le
  Bloc-notes pour le lire :
  ```xml
  <MonDAWSession version="1">
    <Master gain="0.80"/>
    <Track number="1" file="C:\Users\...\batterie.wav" relativeFile="batterie.wav"
           gain="0.50" pan="-0.70" mute="0" solo="0"/>
  </MonDAWSession>
  ```
- **Enregistrer** : on crée un `juce::XmlElement`, on lui ajoute un enfant par piste
  (`createNewChildElement`), on remplit ses attributs, puis `writeTo` écrit le fichier.
- **Ouvrir** : `juce::XmlDocument::parse` relit le fichier. Pour chaque piste, on vide la ligne
  (`clear`), on recharge le son (`loadFile`) et on remet les réglages avec `strips[i]->setGain(...)`,
  etc. Ces fonctions bougent le fader à l'écran **et** préviennent la piste, comme si tu l'avais
  fait à la main.
- **Les sons ne sont pas copiés** : la session retient seulement où ils sont. Elle garde deux chemins.
  Le chemin complet sert si les sons n'ont pas bougé, et le chemin relatif (« batterie.wav, à côté
  de la session ») sert si tu déplaces tout le dossier. Range donc tes sessions dans le même dossier
  que leurs sons.
- Si un son est introuvable, une fenêtre le signale, et le reste de la session s'ouvre quand même.

**Ce que tu apprends** : lire et écrire des fichiers, le format XML, gérer les erreurs proprement.

**Exercices**

1. Ouvre un `.mondaw` dans le Bloc-notes, change `gain="0.50"` en `gain="0.10"`, enregistre, puis
   rouvre la session dans MonDAW. Le fader a suivi.
2. Ajoute un bouton « Nouveau » qui vide toutes les pistes et remet les réglages par défaut.
   Indice : c'est presque `openSession` sans fichier. Pour chaque piste : `rows[i]->clear()`, puis
   `strips[i]->setGain (0.8f)`, etc.
3. Sauvegarde aussi la position de lecture. Ajoute un attribut `position` à `<Master>` (avec
   `getPosition()`), puis relis-le à l'ouverture et passe-le à `seekAll`.

---

## La suite prévue

| Étape | Ce qu'on ajoute | Pourquoi |
|---|---|---|
| 6 | **Clips sur une ligne de temps** : glisser un son pour le faire démarrer plus tard, et plusieurs sons par piste | C'est la base de l'arrangement, et le premier pas vers la partie création. |
| 7 | **Effets par piste** : égaliseur et compresseur dans chaque tranche | C'est le cœur du mixage façon Pro Tools. |
| 8 | **Tracktion Engine** : moteur de DAW complet (clips, boucles, MIDI, plugins VST) | Pour la vue Session façon Ableton, sans tout réécrire nous-mêmes. |
| 9 | **Enregistrement** depuis un micro ou une carte son | Pour créer tes propres sons. |

L'ordre peut changer selon ce qui te motive le plus.
