# Plan — Plugin `KlattVoice` : synthèse vocale Klatt complète, en C++ + MetaSound

> Accueil : [[00 - Accueil]]

> Statut : **validé** (2026-10-01). Langue de départ : **anglais** (2026-10-02).

## Objectif

Repartir de zéro (la version actuelle `MS_KlattVoice_01a/01b` est une première approche simplifiée) et faire une voix qui **articule vraiment** :

- un synthétiseur de Klatt cascade/parallèle complet, nasales comprises ;
- un **moteur d'articulation** qui enchaîne les phonèmes avec des transitions réalistes (coarticulation), une durée par contexte et une intonation ;
- le tout exposé dans MetaSound et en Blueprint.

`ST_Klatt_Phoneme` / `DT_Klatt_Phonemes` ne servent plus de contrainte : l'inventaire des phonèmes vit dans le code (tables de référence), surchargeable plus tard par une DataTable si tu veux bidouiller.

Aucun `.uasset` n'est touché par moi : c'est toi qui poses les nœuds dans l'éditeur.

### Pourquoi pas « DECtalk » directement

DECtalk est propriétaire. Le code source qui circule en ligne a fuité, et il est inutilisable légalement. Je pars des publications :

- Klatt 1980, *Software for a cascade/parallel formant synthesizer* (JASA) ;
- Klatt & Klatt 1990, source glottale KLGLOTT88 ;
- Klatt 1979 et Allen, Hunnicutt & Klatt, *From Text to Speech: The MITalk System* (1987), pour les règles de durée, de loci et de F0.

C'est une implémentation originale. Le son « DECtalk » vient de ce modèle, plus ces règles.

## Architecture

```
Plugins/KlattVoice/
  KlattVoice.uplugin                 (dépend du plugin Metasound)
  Source/KlattVoice/
    KlattVoice.Build.cs
    Private/Klatt/
      KlattSynth.h/.cpp             DSP pur, par échantillon (aucune dépendance UE)
      KlattPhonemes.h/.cpp          inventaire : cibles F1–F5, B1–B5, nasales, AV/AH/AF, gains parallèles, durées
      KlattArticulator.h/.cpp       phonèmes -> trajectoires de paramètres (coarticulation, durées, F0)
    Private/Metasound/
      MetasoundKlattVoiceNode.cpp   nœud « Klatt Voice » : paramètres -> audio (mode manuel)
      MetasoundKlattSpeakNode.cpp   nœud « Klatt Speak » : chaîne de phonèmes + Trigger -> audio (mode parole)
    Public/KlattVoiceLibrary.h      fonctions Blueprint utilitaires (validation d'une chaîne, durée estimée…)
```

Les trois couches (synthé, inventaire, articulateur) sont indépendantes d'Unreal : je peux les tester hors moteur, et les réutiliser ailleurs.

## 1. Synthétiseur (Klatt 1980 complet)

- **Source voisée** : KLGLOTT88 (quotient d'ouverture, inclinaison spectrale), jitter et shimmer légers.
- **Aspiration (AH)** : bruit modulé par la période glottale.
- **Frication (AF)** : bruit dans la branche parallèle.
- **Branche cascade** (voyelles, sonantes) : F1 → F5, plus une **paire pôle/zéro nasale** (FNP / FNZ). C'est ce qui donne les vraies nasales [m n ŋ] et les voyelles nasalisées.
- **Branche parallèle** (consonnes) : résonateurs F2–F6 à gains A2–A6, plus le bypass (AB). C'est ce qui distingue [s] de [ʃ] et de [f].
- **Rayonnement aux lèvres** (différentiateur), lissage des paramètres à chaque échantillon (pas de clics).

## 2. Inventaire des phonèmes

Un jeu de symboles ASCII façon ARPAbet, en anglais, comme DECtalk ; le français peut venir ensuite. Pour chaque phonème :

- **Voyelles** : cibles de formants et durée intrinsèque.
- **Consonnes** : mode d'articulation, voisement, lieu d'articulation (**locus** de F2/F3), gains parallèles.
- **Occlusives** [p t k b d g] : silence de tenue, burst, VOT et aspiration.
- **Nasales** : FNP/FNZ, murmure nasal, nasalisation de la voyelle voisine.

## 3. Articulateur (ce qui fait parler)

- **Segmentation** : chaque phonème a une phase stable et des transitions vers ses voisins.
- **Coarticulation** : en sortie de consonne, les formants partent du **locus** (lèvres ≈ F2 bas, alvéolaires ≈ 1800 Hz, vélaires selon la voyelle) et glissent vers la voyelle, comme chez Klatt.
- **Durées** : règles de Klatt (allongement en fin de phrase, raccourcissement en groupe de consonnes, voyelle accentuée).
- **Intonation** : déclinaison de F0, accents marqués dans la chaîne (`1` / `2`), montée finale pour les questions.
- **Contrôles globaux** : débit, F0 de base, « voix » (tension glottale, souffle).

Exemple de chaîne : `hh ax l ow1 . w er1 l d .`

## Validation

- Compilation avec `Build.bat`.
- Test hors moteur : une petite commande qui rend un `.wav` dans `Saved/` pour écouter sans ouvrir l'éditeur.
- Dans l'éditeur : un MetaSound de test avec `Klatt Speak`, et Audio Insights pour le spectre.

## Ce que tu auras à faire dans l'éditeur

1. Activer le plugin, puis redémarrer.
2. Créer un MetaSound, poser `Klatt Speak`, lui donner une chaîne de phonèmes et un Trigger.

## Ordre de travail proposé

1. Synthé + voyelles fixes (on valide que ça sonne « voix »).
2. Inventaire complet + articulateur (on valide une phrase).
3. Intonation, réglages de voix, nœud Blueprint.
4. Plus tard : texte → phonèmes, et le français.

## Points à trancher

- ~~Nom du plugin~~ → `KlattVoice` (validé)
- ~~Langue de départ~~ → **anglais** (validé), le français viendra ensuite.
