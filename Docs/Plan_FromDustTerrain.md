# Plan — Terrain « From Dust » (2,5D, GPU)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_PatchTerrain]], [[Plan_Water]], [[Plan_CityTool]], [[Plan_GpuTraces]]
> Source : le doc de l'auteur « Idées d'outils Unreal » (Claude Docs, 2026-10-05).

> Statut : 💭 **idée**. **Système Unreal, pas strictement DWAM.** Rien n'est codé.

## L'idée (auteur)

Génération et terraformation en jeu, sans le Landscape de l'éditeur. Simulation par automates cellulaires, **100 % sur le GPU**, sans reconstruire de mesh sur le CPU. Deux voies : **Niagara** (Simulation Stages, sans C++) ou un **plugin C++** avec des compute shaders RDG.

1. **Données** : des Render Targets float, une couche par canal (R = roche, G = sable, B = eau, A = lave). Un texel = l'épaisseur de chaque couche ; la hauteur = leur somme.
2. **Simulation** : deux textures en *ping-pong* ; règles par voisinage :
   - éboulement : le sable glisse si la pente dépasse l'angle critique ;
   - l'eau coule vers le voisin le plus bas ;
   - eau + lave dans la même cellule → roche.

   Plusieurs passes par frame, car la matière n'avance que d'une cellule par passe.
3. **Rendu** : un plan subdivisé statique, déplacé par World Position Offset ; normales recalculées dans le matériau ; mélange des matériaux selon la couche dominante ; l'eau en second plan.
4. **Routes** : elles s'écrivent comme des masques dans la texture (elles forcent la roche et vident les fluides).
5. **Piège** : le WPO est purement visuel. La physique et les traces ne voient pas le terrain déplacé : il faut relire la hauteur sur le CPU (lecture GPU asynchrone, à basse fréquence) pour la collision.

## Remarques (Claude)

- **C'est un autre paradigme** que l'existant :
  - [[Plan_PatchTerrain]] : des surfaces paramétriques sur CPU, avec collision ;
  - [[Plan_Water]] : des bassins (Fill–Spill–Merge). Ce plan a écarté l'automate cellulaire **global** (coût permanent, tout couplé) et ne le garde que pour une **fenêtre locale** autour du joueur.

  Les deux peuvent coexister, à condition de savoir pour quoi : un autre projet, des zones de terraformation locales, ou un remplacement.
- **L'eau** : « couler vers le voisin le plus bas » est la règle la plus simple. Les **tuyaux virtuels** (Mei et al. 2007, déjà dans [[Plan_Water]]) conservent la masse et donnent un vrai écoulement, avec de l'élan.
- **Les traces** : le plugin `GPUTrace` ([[Plan_GpuTraces]]) ne verrait pas non plus un terrain déplacé par WPO. Même remède : la copie de la hauteur relue sur le CPU.

## Questions ouvertes

- Pour quel usage : un projet à part, des zones de terraformation, ou un remplacement du terrain actuel ?
- Niagara ou plugin C++ ?
