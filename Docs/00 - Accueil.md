---
type: accueil
projet: DWAM
mis_a_jour: 2026-10-02
---

# Coffre de Claude — DWAM (Dwam2225)

Coffre de travail de Claude pour le projet Unreal `Dwam2225`. Il contient les plans, le suivi et les décisions techniques.

- **Versionné avec le code** : un plan et le code qui l'implémente avancent ensemble, sur la même branche.
- **Pour l'ouvrir** : dans Obsidian, *Open folder as vault* sur ce dossier `Docs/`.
- **Coffre de l'auteur** (vision, piliers, brainstorming) : `D:\Nos Documents\AI Engineering DWAm`. **Lecture seule pour Claude.**

## Pour toute session Claude qui arrive ici

L'auteur confie ce coffre aux sessions Claude (2026-10-02) : c'est à elles de le tenir propre et à jour.

1. **Lire cette note en entier** avant de travailler, puis le plan du système concerné.
2. **Un plan par système** (`Plan_<Système>.md`), relié à cette note et à ses voisins par des liens `[[...]]`.
3. **Mettre à jour** le tableau « Systèmes » et le **journal** en fin de session (une ligne datée).
4. **Ne pas réécrire l'histoire** : corriger une erreur, oui ; effacer une décision de l'auteur, non. Marquer ce qui est remplacé (~~barré~~ + nouvelle décision datée).
5. **Le coffre de l'auteur** (`D:\Nos Documents\AI Engineering DWAm`) se lit, mais **on n'y écrit jamais**.
6. **Ne rien inventer** : ce qui est inconnu est marqué `À COMPLÉTER`, et on pose la question.
7. **L'auteur dicte souvent au micro** : interpréter les fautes de transcription avec bienveillance, demander en cas de doute.

## Systèmes

| Système | Plan | État |
|---|---|---|
| Terrain en surfaces paramétriques | [[Plan_PatchTerrain]] | 🔧 Plugin `PatchTerrain` codé. Étape 1 testée ; maillage adaptatif, rampe à raccord auto, value noise et lattice compilés, pas encore testés |
| Eau par bassins | [[Plan_Water]] | 📐 Conçu (Watershed), pas codé |
| Synthèse vocale | [[Plan_KlattVoice]] | 📐 Plan validé, anglais d'abord, pas codé |
| Éclairage maison (DwamLighting) | — | 💭 Objectif listé dans `CLAUDE.md`, pas encore de plan |

**Légende** : ✅ fait · 🔧 en cours · 📐 conçu · 💭 idée

## Liens entre systèmes

- **[[Plan_PatchTerrain]] → [[Plan_Water]]** : le terrain fournit la hauteur du sol et la pente ; l'eau n'en dépend pas.
- **[[Plan_PatchTerrain]] → DwamLighting** : ombres du terrain (champ de distances, horizon map), POI cuits avec MDF.
- **[[Plan_Water]] → DwamLighting** : même texture de hauteur ; même chemin GPU (RDG) que le culling de lumières.
- **Matériaux du terrain** (étape « Matériaux » de [[Plan_PatchTerrain]]) : l'orientation par cellule Voronoi est réutilisée par l'eau (sens du courant).

## Conventions

- Un plan par système, en français ; termes Unreal en anglais.
- Distinguer **décision** (validée par l'auteur, datée) et **proposition**.
- Avant un changement important : plan ici, puis validation de l'auteur.
- Commits sur une branche dédiée, jamais directement sur `master`.

## Journal

| Date | Ce qui a été fait |
|---|---|
| 2026-10-01 | Exploration du projet (Blueprints, State Trees, shaders HLSL). Plans KlattVoice et PatchTerrain. Plugin `PatchTerrain` : canevas, modificateurs, expressions, tampons. |
| 2026-10-02 | Maillage adaptatif, value noise, rampe à raccord auto, lattice. Plan Watershed (Fill–Spill–Merge, quadtree implicite, partage CPU/GPU, détection à 2 m). Commit `268f9da` sur la branche `patchterrain-et-plans`. Création de ce coffre. |
