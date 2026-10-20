# MonDAW

Un DAW maison en C++ avec [JUCE](https://juce.com) : création façon Ableton, mixage façon Pro Tools.

**Étape 1** : une fenêtre qui ouvre un fichier WAV, le joue avec Lecture/Stop, et règle le volume avec un fader.

**Étape 2 (actuelle)** : la forme d'onde du fichier s'affiche, avec une tête de lecture ; un clic dans la forme d'onde déplace la lecture.

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
| `Source/MainComponent.h` | Déclare ce que contient la fenêtre (boutons, fader, moteur audio). |
| `Source/MainComponent.cpp` | Le comportement : ouvrir un fichier, jouer, arrêter, régler le volume. |
| `Source/WaveformView.h/.cpp` | Le composant qui dessine la forme d'onde et la tête de lecture. |

Pour apprendre les bases du C++ en lien avec ce code, lis [BASES_CPP.md](BASES_CPP.md).

## Prochaines étapes

1. Ajouter un vumètre à côté du fader.
2. Passer à plusieurs pistes avec Tracktion Engine (clips façon Ableton, table de mixage façon Pro Tools).
