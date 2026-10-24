# MonDAW

Un DAW maison en C++ avec [JUCE](https://juce.com) : création façon Ableton, mixage façon Pro Tools.

**Étape 1** : une fenêtre qui ouvre un fichier WAV, le joue avec Lecture/Stop, et règle le volume avec un fader.

**Étape 2** : la forme d'onde du fichier s'affiche, avec une tête de lecture ; un clic dans la forme d'onde déplace la lecture. Un bouton Pause s'arrête sans revenir au début.

**Étape 3** : un vumètre stéréo à côté du fader montre le niveau du son en direct.

**Étape 4** : 4 pistes jouées ensemble. En haut, la vue Édition (une forme d'onde par piste, sur une échelle de temps commune). En bas, la console de mixage façon Pro Tools : pour chaque piste Mute, Solo, panoramique, fader et vumètre, plus un Master.

**Étape 5** : les boutons « Enregistrer... » et « Ouvrir... » sauvent et rouvrent une session (`.mondaw`) avec les sons de chaque piste et tous les réglages.

**Étape 6 (actuelle)** : des clips sur une ligne de temps. Chaque piste peut contenir plusieurs sons, qu'on ajoute d'un double-clic, qu'on déplace en les glissant et qu'on supprime d'un clic droit. Une règle graduée et une tête de lecture couvrent toutes les pistes.

## Installer les outils sur Windows (une seule fois)

1. **Visual Studio 2022 Community** (gratuit) : https://visualstudio.microsoft.com/fr/vs/community/
   Pendant l'installation, coche la charge de travail **« Développement Desktop en C++ »**.
   Elle contient le compilateur C++ et CMake.
2. **Git** : https://git-scm.com/download/win (options par défaut).

## Compiler et lancer

Après une mise à jour du code, si Visual Studio ne voit pas un nouveau fichier : menu **Projet > Supprimer le cache et reconfigurer**.

Ouvre **« Developer PowerShell for VS 2022 »** (menu Démarrer), puis :

```powershell
cd chemin\vers\mon-daw
cmake -B build
cmake --build build --config Debug
.\build\MonDAW_artefacts\Debug\MonDAW.exe
```

Le premier `cmake -B build` télécharge JUCE automatiquement, il peut prendre quelques minutes.

Plus simple encore : dans Visual Studio, **Fichier > Ouvrir > Dossier** sur `mon-daw`. Visual Studio reconnaît le `CMakeLists.txt` ; choisis `MonDAW.exe` comme cible de démarrage puis appuie sur **F5**.

## Organisation du code

| Fichier | Rôle |
|---|---|
| `CMakeLists.txt` | La « recette » de construction : récupère JUCE et déclare l'application. |
| `Source/Main.cpp` | Point d'entrée : crée la fenêtre. |
| `Source/MainComponent.h/.cpp` | La fenêtre : barre de transport, disposition, et le moteur de mixage (`getNextAudioBlock`). |
| `Source/Track.h/.cpp` | Le « modèle » d'une piste : ses clips et ses réglages (volume, panoramique, mute, solo). |
| `Source/Clip.h/.cpp` | Un clip : un son chargé en mémoire et sa position de départ. |
| `Source/Timeline.h` | La conversion temps ↔ pixels, la règle graduée et la ligne de tête de lecture. |
| `Source/TrackRow.h/.cpp` | Une ligne de la vue Édition : nom de piste, bouton « Ajouter un son... » et la bande des clips. |
| `Source/ClipComponent.h/.cpp` | Un clip à l'écran : forme d'onde, glisser pour déplacer, clic droit pour supprimer. |
| `Source/MixerStrip.h/.cpp` | Une tranche de la console : Mute, Solo, panoramique, fader, vumètre. |
| `Source/LevelMeter.h/.cpp` | Le vumètre : reçoit le niveau depuis le thread audio et le dessine. |

Pour comprendre chaque étape en détail, avec des exercices, lis [GUIDE_ETAPES.md](GUIDE_ETAPES.md). Pour les bases du C++ en lien avec ce code, lis [BASES_CPP.md](BASES_CPP.md).

## Prochaines étapes

Voir la fin de [GUIDE_ETAPES.md](GUIDE_ETAPES.md#la-suite-prévue).
