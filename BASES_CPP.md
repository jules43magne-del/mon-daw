# Les bases du C++, expliquées avec le code de MonDAW

Ce guide n'essaie pas de tout couvrir. Il explique juste ce qu'il faut pour **lire et modifier** le code du projet. Chaque notion renvoie à un endroit réel du code.

> Les exemples viennent de la version de l'étape 1. Depuis l'étape 4, le lecteur est devenu multipiste : les mêmes idées se retrouvent dans `Track.cpp`, `MixerStrip.cpp` et `MainComponent.cpp`.

## 1. Compiler, c'est traduire

Le C++ est un langage **compilé** : on écrit du texte (`.cpp`, `.h`), un programme appelé *compilateur* le traduit en `.exe`. Si le code contient une faute, la compilation échoue et te dit à quelle ligne. C'est normal et fréquent : lis le **premier** message d'erreur, c'est souvent le seul qui compte.

## 2. Les variables et leurs types

Une variable est une boîte nommée qui contient une valeur. En C++, chaque boîte a un **type** fixe.

```cpp
int nombrePistes = 4;        // nombre entier
float volume = 0.8f;         // nombre à virgule (le f = float)
double sampleRate = 48000.0; // nombre à virgule plus précis
bool enLecture = false;      // vrai ou faux
juce::String nom = "Kick";   // du texte (type fourni par JUCE)
auto x = 3;                  // "auto" : le compilateur devine le type (ici int)
```

Dans le projet : `const bool hasFile = readerSource != nullptr;` dans `updateButtons()`.
`const` veut dire « cette valeur ne changera plus ».

## 3. Les conditions

```cpp
if (volume > 1.0f)
    volume = 1.0f;
else
    volume = volume * 2;
```

`!` veut dire « non », `&&` « et », `||` « ou ». Dans le projet :
`playButton.setEnabled (hasFile && ! playing);` → le bouton Lecture est actif si un fichier est chargé **et** que ça ne joue **pas**.

## 4. Les boucles

```cpp
for (int i = 0; i < 4; ++i)
{
    // s'exécute 4 fois, avec i = 0, 1, 2, 3
}
```

En audio, on boucle sans arrêt sur les échantillons (les petits nombres qui forment le son). Tu en écriras bientôt pour le vumètre.

## 5. Les fonctions

Une fonction regroupe des instructions sous un nom, peut recevoir des paramètres et renvoyer un résultat.

```cpp
float doubler (float valeur)
{
    return valeur * 2.0f;
}
```

Dans le projet : `void MainComponent::openButtonClicked()`. `void` signifie « ne renvoie rien ».

## 6. Les classes et les objets (le cœur du C++)

Une **classe** est un plan de fabrication ; un **objet** est une chose construite avec ce plan. Une classe regroupe des **données** (variables membres) et des **actions** (fonctions membres, appelées *méthodes*).

`MainComponent` est une classe. Ses données : les boutons, le fader, le lecteur audio. Ses méthodes : `resized()`, `paint()`, `getNextAudioBlock()`...

- On appelle une méthode avec un point : `transportSource.start();`
- Le **constructeur** `MainComponent::MainComponent()` s'exécute à la création de l'objet : c'est là qu'on prépare l'interface.
- Le **destructeur** `~MainComponent()` s'exécute à sa destruction : c'est là qu'on coupe l'audio.

### Héritage

`class MainComponent : public juce::AudioAppComponent` veut dire : « MainComponent **est un** AudioAppComponent, avec en plus mes propres ajouts ». On récupère gratuitement tout ce que JUCE a déjà écrit (fenêtre, accès à la carte son).

Le mot `override` indique qu'on **remplace** une méthode prévue par JUCE. Par exemple JUCE appelle `getNextAudioBlock()` des centaines de fois par seconde pour réclamer du son ; nous on décide quoi lui donner.

## 7. `.h` et `.cpp`

- Le `.h` (*header*) **déclare** : « voici ce que contient la classe ».
- Le `.cpp` **définit** : « voici comment ça marche ».

`#include "MainComponent.h"` copie le header au début du `.cpp` pour que le compilateur connaisse la classe.

## 8. Les pointeurs, en version simple

Un pointeur contient **l'adresse** d'un objet plutôt que l'objet lui-même. `nullptr` veut dire « ne pointe sur rien ».

On n'utilise presque jamais de pointeurs « nus » : on préfère `std::unique_ptr`, qui libère automatiquement la mémoire.

```cpp
std::unique_ptr<juce::AudioFormatReaderSource> readerSource; // vide au départ
readerSource = std::make_unique<...>(...);                  // on crée l'objet
if (readerSource == nullptr) { /* pas encore de fichier */ }
```

`->` sert à appeler une méthode à travers un pointeur : `chooser->launchAsync(...)`.

`&` devant une variable donne son adresse (`&transportSource`). Dans un paramètre, `const juce::FileChooser& fc` est une **référence** : on travaille sur l'objet original sans le copier.

## 9. Les lambdas

Une lambda est une petite fonction écrite sur place, souvent pour réagir à un clic :

```cpp
playButton.onClick = [this] { transportSource.start(); };
```

« Quand on clique, lance la lecture. » Le `[this]` permet à la lambda d'accéder aux membres de la classe.

## 10. La règle d'or de l'audio temps réel

`getNextAudioBlock()` tourne sur un fil d'exécution spécial, le **thread audio**. Il doit rendre la main très vite, sinon on entend des craquements. Donc, dans cette fonction : pas d'ouverture de fichier, pas d'allocation mémoire, pas d'attente. Tout le travail lourd se fait ailleurs (comme l'ouverture du fichier dans `openButtonClicked()`).

---

## Exercices pour t'entraîner sur le projet

1. Change le texte du bouton « Lecture » en « ▶ Play » (dans `MainComponent.h`).
2. Change la taille de la fenêtre (cherche `setSize`).
3. Mets le volume de départ à 50 % au lieu de 80 %.
4. Ajoute un bouton « Pause » qui arrête la lecture **sans** revenir au début (indice : copie le bouton Stop sans `setPosition (0.0)`).

Recompile après chaque changement pour voir le résultat.

## Pour aller plus loin

- https://www.learncpp.com : le meilleur cours gratuit (en anglais), chapitres 0 à 13 en priorité.
- https://zestedesavoir.com/tutoriels/822/la-programmation-en-c-moderne/ : cours en français sur le C++ moderne.
- https://juce.com/learn/tutorials : les tutoriels officiels JUCE (« Getting started », puis « Audio »).
