# Plan — Plugin `PatchTerrain` : terrain en surfaces paramétriques, par chunks

> Statut : **validé, étape 1 en cours** (2026-10-01).

## Objectif

Remplacer le Landscape par un terrain fait de **surfaces lisses définies par des points de contrôle**, qui :

- s'édite à la main en éditeur (handles, formules par zone) ;
- se **génère et varie aussi dans un build packagé** (seed) ;
- se découpe en **chunks** pour le streaming ;
- est conçu comme un système générique (nom neutre), réutilisable hors de ce projet.

Références de jeu : caméra subjective ; régions façon *Fable 1/2/3* (vallons, montagnes et falaises qui servent de murs naturels) reliées par de la forêt ou de la plaine ; à terme, assemblage de pièces façon *Diablo 2*.

## Principes

### 1. Un canevas commun, pas des patchs indépendants

Tous les points de contrôle vivent sur **un seul canevas** :

- c'est une grille rectilinéaire, dont chaque colonne a sa largeur et chaque ligne sa hauteur, comme un tableur ;
- un « patch », c'est une case de ce canevas ; une case de 10 × 23 m, c'est une colonne et une ligne larges ;
- on interdit les jonctions en T (une case qui chevaucherait la grille à moitié) ; pour du détail local, on **subdivise** une ligne ou une colonne.

Comme les cases partagent les mêmes points, la continuité est garantie : **rien à recoudre**.

### 2. Type de surface

- **Étape 1 : Catmull-Rom non uniforme bicubique.** La surface **passe par les points de contrôle** : un handle déplacé, c'est la surface qui bouge à cet endroit. Elle est continue en pente (C1), y compris quand les cellules ont des tailles différentes.
- **Option plus tard : B-spline cubique.** Encore plus lisse (C2), mais les points de contrôle « flottent » au-dessus de la surface. On l'ajoutera comme mode alternatif si le besoin se fait sentir.

### 3. Pile de modificateurs (les « formules »)

Une liste ordonnée de modificateurs s'applique au-dessus de la surface de base. Chacun a :

- **une zone** : tout le terrain, ou un rectangle de cases, avec un fondu en bordure ;
- **une fonction** :
  - **Presets** (étape 1) : Vagues, Bruit fBm, Crêtes (ridged), Dunes, Terrasses ;
  - **Expression libre** (fait, étape 1) : un petit parseur maison (`sin(x/500)*200 + noise(x,y)*50`) qui fonctionne aussi en package, sans compilation de shader.

La pile est déterministe (même seed → même terrain, en éditeur comme en package).

### 3 bis. Tampons (stamps) : l'outil principal

Un acteur `Patch Terrain Stamp` qu'on glisse sur le terrain et qu'on manipule avec les gizmos standard :

- position et rotation (lacet) via le gizmo ; le scale X/Y donne l'emprise, le scale Z la hauteur ;
- **un menu « Forme »** pour changer de forme sans perdre le placement :
  - relief : Colline, Plateau, Rampe, Crête, Vallon, Cratère, Volcan, Dunes ;
  - mathématiques : Boîte à œufs, Selle, Ondulations, Pyramide, Cône ;
  - Expression (formule libre) ;
- **contour** de rond à carré, et **bords doux** réglables ;
- **mélange** : Ajouter, Creuser, Max (fusionner sans additionner), Min, Aplatir (plateaux, routes, rampes taillées dans le relief) ;
- **détails** : marches douces (nombre, douceur) et bruit additionnel (doux, crêtes, bosses), ancrés au tampon ;
- une boîte filaire montre l'emprise et la hauteur ;
- un terrain cible optionnel (sinon : tous les terrains touchés) et une priorité d'application.

Ordre d'évaluation : canevas → modificateurs (coordonnées du canevas) → tampons (position réelle de la surface).
Les tampons sont des acteurs du niveau : ils marchent en package, et le terrain les relit au lancement.

- **Raccord auto** (Rampe) : la rampe relie la hauteur du terrain sous ses deux extrémités (avant elle dans la pile) ; pas de fondu aux extrémités, seulement sur les côtés.
- Bruits : doux (fBm), crêtes, bosses, **valeur** (value noise).

Plus tard (module éditeur) : poignées sur les arêtes et les coins pour étirer un seul côté.

### 4. Lattice (en cours)

Acteur `Patch Terrain Lattice` : une boîte de déformation (FFD de Bézier, grille de 2 à 5 points par axe) appliquée en dernier, sur la position 3D. Les points de contrôle se déplacent dans le viewport ; un fondu aux faces de la boîte garantit la continuité avec le terrain non déformé. Le terrain est stocké comme une **surface 3D** et non comme une heightmap : la lattice reste possible sans rien refaire.

## Architecture

```
Plugins/PatchTerrain/
  PatchTerrain.uplugin
  Source/PatchTerrain/                  (module Runtime : fonctionne en éditeur ET en package)
    PatchTerrain.Build.cs               (Core, CoreUObject, Engine, GeometryCore, GeometryFramework)
    Public/PatchTerrainTypes.h          modificateurs (structs/enums)
    Public/PatchTerrainSurface.h        maths pures : évaluation de la surface + pile de modificateurs
    Public/PatchTerrainActor.h          APatchTerrain : données du canevas, chunks, génération des meshes
    Private/...
  (plus tard) Source/PatchTerrainEditor/  outils d'édition dédiés (grille visible, insérer ligne/colonne, peinture…)
```

Le rendu passe par des `UDynamicMeshComponent`, un par chunk : ils sont runtime, avec collision et matériau standard.

## Étapes

### Étape 1 — Le cœur (en cours)

- Canevas : largeurs de colonnes, hauteurs de lignes, points de contrôle 3D.
- **Handles dans le viewport** : chaque point de contrôle est un gizmo déplaçable (`MakeEditWidget`).
- Évaluation Catmull-Rom non uniforme, et presets de modificateurs.
- Découpage en chunks (N × M cases), maillage avec une densité réglable en points par mètre, normales analytiques (aucune couture entre chunks), UV monde et tangentes.
- Reconstruction automatique en éditeur, et génération au `BeginPlay` en jeu (le terrain est toujours recalculé depuis ses données, pas sauvegardé en mesh).
- Collision (complexe = simple), cuisson asynchrone en jeu.
- Seed, et option « seed aléatoire au lancement » pour tester les variations.

### Étape 1 bis — Qualité du maillage (en cours)

- Densité adaptative : chaque carré de base est subdivisé (quadtree, jusqu'à 2^N) là où la surface s'écarte de l'interpolation de ses coins au-delà d'une tolérance (cm).
- Pas de fissure : chaque feuille est triangulée en éventail en incluant tous les sommets présents sur ses bords ; les bords de chunk sont toujours échantillonnés à la résolution la plus fine (identiques des deux côtés).

### Étape 2 — Édition confortable

- Insérer ou supprimer une ligne ou une colonne en gardant les formes.
- Dessin de la grille dans le viewport, et sélection d'une case pour y poser un modificateur.
- ~~Expression libre (parseur)~~ : fait à l'étape 1.
- Ne reconstruire que les chunks touchés.

### Étape 2 bis — Matériaux (« overlay » organique)

But : une base triplanaire (roche) et, par-dessus, des zones (sable, chemins, autres roches) qui se chevauchent avec un bord franc et lisible, comme l'overlay des Connected Textures de Minecraft, **sans grille visible**.

- **Cellules de Voronoi comme tuiles** : une grille invisible, un point aléatoire par case (jitter réglable, du presque carré au très organique). Chaque cellule porte un identifiant de matériau.
- **Texture de données** (dans l'espace du canevas, un texel par cellule) : identifiant de matériau + direction de pente au centre de la cellule ; calculée au rebuild sur le CPU (déterministe avec la seed, donc OK en package).
- **Shader** : Voronoi 3×3 (F1, F2 − F1) → cellule courante, voisine et distance à la frontière ; **overlay par priorité** (le matériau prioritaire déborde sur une bande), avec une lèvre stylisée (liseré, assombrissement, relief en normal map) ; mélanges en OKLab.
- **Orientation par cellule** (d'après le shader de flowmap Voronoi de l'utilisateur) : chaque cellule fait tourner sa texture en bloc selon la vraie pente du terrain, `-(N.x, N.y)`. Pas d'étirement ; jointures sur les frontières. Usages : eau, rides de sable, coulures, herbe couchée, strates.
- **Sources des identifiants** : règles (pente, altitude, bruit), tampons de matériau (même ergonomie que les tampons de relief), chemins sur spline (forme continue par distance au bord, qui réécrivent aussi les cellules traversées), et plus tard un pinceau (module éditeur).
- Les falaises (pente forte) passent en matériau dédié en placage triplanaire ; l'overlay peut border leur sommet.
- Plus tard : accumulation d'écoulement (où l'eau se rassemble) pour placer rivières et sols humides.

### Étape 3 — Runtime et streaming

- Génération minimale au chargement, puis streaming de chunks autour du joueur (maillage et collision en tâche de fond).
- Navmesh dynamique avec des invokers.
- Variations procédurales : modificateurs à paramètres aléatoires bornés, choix entre variantes.

### Étape 4 — Ombres (raccord avec `DwamLighting`)

Un mesh dynamique **n'a pas de Mesh Distance Field** (fonctionnalité dépréciée par Epic en 5.0), donc le terrain est invisible pour le ray-march SDF actuel. Les pistes, de la recommandée à la plus simple :

1. **Champ de distances du terrain, calculé par chunk** (compute shader), au chargement du chunk ; le shader d'ombre prend `min(GDF, champ terrain)`. Le terrain bouge rarement : on le calcule une seule fois.
2. **Horizon map** : on précalcule l'angle de l'horizon dans N directions, et l'ombre du soleil devient un simple test. Très rapide, idéal pour le soleil.
3. **Ray-march d'une heightmap** du terrain, accélérée par mip min/max.
4. **Proxys à MDF** : des primitives invisibles (avec leur MDF) qui approximent le relief. C'est la solution de secours la plus simple.

Le module d'ombre vivra côté `DwamLighting` (sur mesure) : `PatchTerrain` se contentera d'exposer les données (hauteurs ou champ de distances) de façon neutre.

### Étape 4 bis — POI cuits

Les pièces faites main (points d'intérêt) sont converties en Static Mesh dans l'éditeur et obtiennent leur Mesh Distance Field. Le MDF appartient au mesh, pas à sa position : le générateur peut placer, tourner et combiner ces pièces au hasard (variations du puzzle) sans perdre les ombres SDF. Seul le terrain procédural entre les POI dépend de la texture de hauteur et du champ de distances du terrain.

### Étape 5 — Pièces façon Diablo 2

Des pièces conçues à la main (des morceaux de canevas avec leurs modificateurs), aux **bords standardisés** (profils de connexion), assemblées par un générateur.

## Ce que tu fais dans l'éditeur (étape 1)

1. Fermer l'éditeur, compiler, rouvrir (le plugin est activé par défaut).
2. Placer un `Patch Terrain` dans un niveau, lui donner un matériau, et bouger les handles.

## Points ouverts

- Densité de maillage par défaut, et taille de chunk (à régler à l'usage).
- Convention des tangentes pour tes normal maps (à vérifier avec ton TBN maison).
