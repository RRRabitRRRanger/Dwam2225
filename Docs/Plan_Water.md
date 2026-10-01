
| Taille de cellule | 2 m par défaut et minimum. Les courbes volume/niveau peuvent être calculées depuis le terrain à une résolution plus fine : la grille ne limite pas la précision des niveaux |# Plan — Watershed : système d'eau par bassins (plugin générique)

> Statut : **conception**, en attente de validation. Rien n'est codé.

## Le problème de départ

Le prototype `Content/AVOLABO/Water/BP_WaterGridManager` (avril 2026) est un automate cellulaire en voxels :

- un BoxTrace par cellule pour savoir si elle est libre ;
- l'eau qui tombe, puis s'étale aux 4 voisins (von Neumann) ;
- des cellules stockées dans un tableau cherché linéairement.

L'idée est bonne, mais le prototype est impraticable :

- **une seule résolution pour tout** : une flaque et la mer coûtent pareil au m² ;
- **tout est couplé** : un obstacle oblige à tout resimuler ;
- **un lac au repos coûte autant qu'une rivière** ;
- **la structure de données est en N²** (recherche linéaire à chaque accès).

L'intuition de l'utilisateur, trouvée seul : **ce qui compte, ce n'est pas l'eau case par case, c'est un niveau par poche**, et ce niveau dépend de la forme (cuve ≠ cône), tranche par tranche. Ça permet de se débarrasser des cases tout en gardant l'essentiel de l'information.

Référence qui formalise exactement cette idée :

- Barnes, Callaghan & Wickert (2021), *Computing water flow through complex landscapes – Part 3: Fill–Spill–Merge*, Earth Surface Dynamics 9, 105–121 — https://esurf.copernicus.org/articles/9/105/2021/
- Code : https://github.com/r-barnes/Barnes2020-FillSpillMerge

## Principe : une hiérarchie de bassins

Si on fait monter l'eau partout en même temps, des poches apparaissent, puis fusionnent en passant des cols : c'est un **arbre**.

Chaque nœud est un bassin, avec :

- **une courbe volume ↔ niveau**, calculée tranche par tranche à partir des surfaces (cuve : linéaire ; cône : non linéaire) ;
- **une hauteur de débordement** (le col) et le bassin où il déborde ;
- **son volume d'eau actuel**, d'où l'on déduit son niveau.

Les opérations :

- **Ajouter ou retirer de l'eau** : on se déplace sur la courbe. Au-delà du col, le surplus part au voisin ou au parent (*Fill–Spill–Merge*).
- **Un obstacle apparaît ou disparaît** : on recalcule seulement les bassins dont l'emprise est touchée, puis on redistribue leur eau.
- **Rivières** : des débits portés par les arêtes de débordement, et l'accumulation d'écoulement le long des pentes (commune avec les matériaux du terrain).

## Données

| Donnée | Forme |
|---|---|
| Hauteur du sol (2,5D) | Grille dense (texture), fournie exactement par le terrain : aucune trace |
| Occupation 3D (cavernes, bâtiments, objets) | Seulement dans des **volumes délimités** ; grille de voxels remplie sur GPU (Global Distance Field), relue en asynchrone |
| Étiquettes | Grille dense cellule → bassin (accès direct) |
| Bassins | Nœuds : courbe volume/niveau (tableau trié), col, parent, enfants, volume d'eau |
| Arêtes | Débordements : hauteur, débit |

Pour les cavernes, on applique le même principe en 3D : on balaie le volume vide par tranches.

**Scan des cavernes** : par étages, du fond vers le haut. Un étage traité est mis en cache et n'est plus retouché, sauf gros changement détecté. Le GDF n'existant qu'autour de la caméra (clipmap 0 : 25 m de rayon, voxels d'environ 20 cm avec Lumen, `r.LumenScene.GlobalSDF.Resolution = 252`), une caverne est scannée la première fois que le joueur s'en approche. Les cavernes des POI cuits sont scannées dans l'éditeur, directement depuis les meshes. **Échantillonnage à l'échelle des cellules (2 m), pas du GDF** : en dessous d'un personnage, une poche ne compte pas pour l'eau. On lit la distance au centre de chaque cellule : inférieure à environ une demi-cellule (1 m), la cellule est pleine. Un champ de distances rend cet échantillonnage grossier fiable : une paroi fine (20 cm) est à moins d'1 m du centre de sa cellule, donc détectée, sans fuite. En 3D, c'est environ 1000 fois moins d'échantillons qu'à la résolution du GDF.

**Air piégé** (choisi) : chaque bassin sait s'il a une issue vers le haut. Sans issue, l'air ne peut pas sortir, donc l'eau entrée par le bas s'arrête au-dessus du passage d'entrée (effet cloche de plongée). On obtient des poches d'air respirables et des salles sèches derrière un siphon.

## Exécution (asynchrone)

1. **Détection** (GPU) : occupation des volumes 3D par échantillonnage du GDF ; lecture GPU → CPU asynchrone.
2. **Hiérarchie** (thread de fond, UE::Tasks) : remplissage par priorité (*priority flood*), étiquetage, arbre, courbes.
3. **Distribution de l'eau** (thread de fond) : Fill–Spill–Merge.
4. **Application** (thread de jeu) : échange du résultat en une fois (double buffer).
5. **Mises à jour locales** : un obstacle ou une source marque une zone sale ; seuls les bassins concernés repassent par les étapes 2 et 3.

## Rendu et détail

- **Lacs et mers au repos** : un simple plan ne suffit pas (il flotte au-dessus d'un bassin voisin plus bas, et traverse les cavernes). Chaque bassin est couvert par un **quadtree par excès** construit sur la grille d'étiquettes : grandes tuiles au centre, petites au rivage, LOD naturel au loin. Les tuiles (carrés de tailles 2^k) sont rendues en **HISM**. Le shader découpe le débord au pixel avec la profondeur (niveau − sol) et l'étiquette du bassin : rivage exact, sans escalier.
- **Eau en mouvement** (près du joueur seulement) : une simulation GPU locale en « tuyaux virtuels » (Mei et al. 2007, version continue de l'automate de von Neumann), dans une fenêtre qui suit le joueur : la moitié de la clipmap 0 du Global Distance Field, soit 1250 uu de rayon (25 × 25 m). Elle est initialisée depuis les niveaux des bassins et leur rend les volumes.
- **Matériaux** : la vitesse du courant alimente l'orientation par cellule Voronoi (flowmap) du terrain.
- **Effets** (écume, éclaboussures) : Niagara, qui lit les textures d'eau.

## Partage CPU / GPU

Le critère est la nature du calcul : ce qui est séquentiel et hiérarchique va sur CPU, ce qui est massivement parallèle par cellule va sur GPU.

| Partie | Où | Pourquoi |
|---|---|---|
| Arbre de bassins, niveaux, air piégé, Fill–Spill–Merge | CPU, thread de fond (C++) | Algorithmes séquentiels (priority flood, fusions aux cols, parcours d'arbre) |
| Détection des volumes 3D (cavernes) | **Niagara** (data interface Global Distance Field) | Un échantillon par cellule, en parallèle |
| Simulation locale qui suit le joueur | **Niagara** (Grid2D, simulation stages) | Tuyaux virtuels = calcul par cellule |
| Tuiles des lacs | CPU → HISM d'abord ; **Niagara** (mesh renderer) si les niveaux bougent en continu | Voir « Quadtree implicite » |
| Effets (écume, éclaboussures) | **Niagara** | Natif |
| Retour gameplay (nager, flotter) | Lecture GPU → CPU asynchrone | Quelques frames de retard, accepté |

Les systèmes Niagara sont des `.uasset` : l'utilisateur les assemble dans l'éditeur. Le HLSL de chaque étape est écrit en texte et versionné dans le plugin, avec un pas-à-pas de câblage. À vérifier : Niagara 5.8 peut-il inclure directement les `.ush` du plugin (sinon, code collé dans des nœuds Custom HLSL) ?

## Quadtree implicite (tuiles des lacs)

Pas d'arbre à pointeurs : une **pyramide de mips** de la grille d'étiquettes.

- Niveau 0 : une cellule (2 m) = l'étiquette de son bassin.
- Niveau k+1 : chaque texel prend l'étiquette de ses 4 enfants s'ils sont identiques, sinon « mixte ».
- **Émission d'une tuile** : texel uniforme dont le parent est mixte (la plus grande tuile possible) ; au niveau 0, les cellules mixtes du rivage sont émises aussi (approximation par excès), puis découpées au pixel par le shader (profondeur + étiquette).
- **LOD** : au loin, on autorise l'émission plus haut dans la pyramide.
- Chaque texel décide seul (ses enfants, son parent) : parallèle par nature.

Mise en œuvre : **sur CPU d'abord** (construction quand un bassin change, en tâche de fond ; tuiles → HISM, avec culling et collision fournis). La même logique passera telle quelle sur GPU (Niagara : une particule par tuile, mesh renderer) quand les niveaux bougeront en continu (crues, vidanges).

## Budget estimé (à mesurer)

Exemple : région de 2 km × 2 km en cellules de 2 m (1 M de cellules).

- Grille simulée partout, pour comparaison : quelques secondes par frame en Blueprint, 10 à 20 ms en C++, 0,2 à 0,5 ms sur GPU, payés en permanence.
- Watershed :
  - arbre de bassins : 0,1 à 0,5 s par région, en tâche de fond ;
  - mises à jour locales : quelques ms, en tâche de fond ;
  - Fill–Spill–Merge sur événement : quelques microsecondes ;
  - pyramide du quadtree : quelques ms, en tâche de fond ;
  - fenêtre GPU (25 × 25 m, 625 à 2 500 cellules) : négligeable ;
  - environ 10 Mo de données par région.
- Objectif en régime stable : moins de 0,2 ms CPU et moins de 0,5 ms GPU par frame (hors shading de l'eau).
- Risques : streaming des régions, raccord fenêtre ↔ bassins, coût du shader d'eau.

## Liens avec les autres systèmes

- **PatchTerrain** : il fournit la hauteur du sol et la pente (texture), et notifie quand une zone change. L'eau ne dépend pas du terrain : n'importe quelle source de hauteur convient.
- **DwamLighting** : la même texture de hauteur sert au ray-march d'ombre de heightmap.
- Le chemin GPU (RDG, compute shaders) est le même que pour le culling de lumières.

## Étapes proposées

1. **Hiérarchie 2,5D sur CPU** à partir de la hauteur du terrain : bassins, cols, courbes ; debug visuel (niveaux, cols, arbre).
2. **Fill–Spill–Merge** : sources d'eau, débordements, rendu simple des lacs.
3. **Mises à jour locales** (obstacles, sources) en asynchrone.
4. **Volumes 3D** (cavernes) avec l'occupation détectée sur GPU.
5. **Simulation locale GPU** et rendu de l'eau en mouvement.

## Décisions

- Nom : **Watershed**.
- Air : **piégé** (une fuite lente pourra s'ajouter plus tard).
- Cellules : 2 m par défaut et minimum.
- Fenêtre simulée : moitié de la clipmap 0 du GDF = 1250 uu de rayon (25 × 25 m), cellules de 50 cm à 1 m (625 à 2 500 cellules) ; les rides fines se font dans le shader.
