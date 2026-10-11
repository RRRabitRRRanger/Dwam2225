# Plan — Un objet « graphe » générique (remplaçant de la spline)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_CityTool]], [[Plan_ProceduralTrees]], [[Plan_Fire]], [[Plan_EireneNexus]], [[Plan_Water]]

> Statut : 🔧 **v0 codée** (2026-10-05) : plugin `GraphKit`, 3 tests automatiques réussis ; guide [[Guide_LayoutKit]]. **Outil Unreal, pas strictement DWAM.** Pas encore vu dans l'éditeur.

## Le constat (auteur, 2026-10-05)

Plusieurs projets de l'auteur devront utiliser **un graphe**, d'une manière ou d'une autre. Il n'avait pas encore prévu d'**objet graphe**, par exemple comme remplaçant de la spline.

## Qui en a besoin (déjà dans les notes)

| Usage | Graphe | Spatial ? |
|---|---|---|
| Outil ville ([[Plan_CityTool]]) | Routes : points (anchors), tronçons, embranchements | Oui |
| Chaufferie, réseaux (proto `EUW_Graph`) | Tuyaux, vannes, débits | Oui, ou abstrait |
| Arbres ([[Plan_ProceduralTrees]]) | Tronc, branches, bourgeons ; coupé, repousse | Oui |
| Feu ([[Plan_Fire]]) | Nœuds de combustible, construits autour du feu | Oui |
| Factions ([[Plan_EireneNexus]]) | Relations entre groupes et personnages | Non (abstrait) |
| Eau ([[Plan_Water]]) | L'arbre des bassins, les débordements | Implicite |

## Proposition (Claude) : trois couches

1. **Les données** (générique, pour tous les projets) : des **nœuds** et des **arêtes**, chacun avec un identifiant stable (GUID), des **tags** et une **charge utile** libre (une struct). Orienté ou non. Aucune hypothèse sur l'usage.
2. **La couche spatiale** (le remplaçant de la spline) : les nœuds ont une transformation (les **anchors**), les arêtes une **courbe** (droite, Bézier, lissée), et les **embranchements** sont naturels. Une API d'échantillonnage : un point à une distance donnée, la tangente, le repère le long d'une arête, comme une spline, mais à travers un graphe.
3. **Les vues et les outils** :
   - dans la scène : les poignées, l'édition façon Blender ([[Plan_CityTool]]) ;
   - en abstrait : `EUW_Graph`, la vue « vue de haut » en nœuds ;
   - en jeu : le graphe reste modifiable (un arbre coupé, un feu qui avance).

Chaque projet ajoute **sa** charge utile et **ses** règles (débit pour la chaufferie, rayon pour les arbres, combustible pour le feu) sans toucher au socle.

## Version 0 : le strict minimum (proposition, en attente de validation)

**Plugin à part, réutilisable**, nom : **`GraphKit`** (validé par l'auteur, 2026-10-05). Un seul module, runtime, pas encore de module éditeur.

**Dans la v0 (le socle, sans position)** :
- **Un graphe** qui contient :
  - des **nœuds** : un **nom** unique dans le graphe, des **tags** (Gameplay Tags), une **charge utile** libre (Instanced Struct) ;
  - des **arêtes** : le nom du nœud de départ, le nom du nœud d'arrivée, une **distance**, des tags, une charge utile.
  - **Pourquoi des noms plutôt que des GUID** (Claude, 2026-10-05) : les graphes s'écrivent **à la main** ([[Plan_DreamPlaces]]) ; « Louvre » ou « Entrée » se lisent et se tapent, un GUID non. Comme les noms sont uniques **dans** un graphe, le copier-coller reste sûr. Un GUID viendra s'il faut un jour fusionner des graphes.
  - Un réglage du graphe : **orienté ou non**.
- **L'API, appelable en Blueprint** : ajouter ou retirer un nœud (retirer un nœud retire ses arêtes), ajouter ou retirer une arête, trouver un nœud par son identifiant, lister les voisins d'un nœud (avec la distance), lister les nœuds ou arêtes portant un tag.
- **Où vit le graphe** : c'est une **struct** (`FGraphKitGraph`), qu'on pose partout (variable d'acteur, Data Asset, composant). Une bibliothèque Blueprint la lit et la modifie par référence.
- *(Retiré de la v0 par la décision ci-dessous : la position, les poignées dans la scène, l'affichage en lignes, le point le long d'une arête. Ils passent dans la couche spatiale.)*
- **Copier / coller** : les identifiants sont uniques **dans un graphe**, pas dans tout le monde. Un graphe dupliqué garde donc des références valides, sans remappage. Le partage de points entre graphes viendra plus tard.
- **Des tests automatiques** sur les opérations (comme pour Watershed et Rope).

**Décision (auteur, 2026-10-05) : pas de position dans le socle, des distances.** Les **arêtes** portent une **distance** (un poids) ; les nœuds n'ont pas de position. Pour le feu, les arbres et les factions, la position est secondaire ; pour une chaufferie, elle est très pratique mais **mathématiquement inutile** pour tout résoudre. La position n'existe que dans une **couche spatiale optionnelle**, où la distance d'une arête est **calculée** à partir de la géométrie (sauf si on la fixe à la main). **La v0 est le socle seul** : nœuds, arêtes, distances, tags, charges utiles, API, tests. La couche spatiale et les poignées dans la scène viennent ensuite.

L'auteur a **une idée pour les chemins** : des **lieux rêvés** qui varient sans perdre leur identité ([[Plan_DreamPlaces]]). Le graphe sans position y devient un principe : **le graphe porte l'identité d'un lieu, la géométrie porte la variation**.

**Hors v0, pour plus tard** : courbes (Bézier, lissage), plus court chemin et autres algorithmes, édition façon Blender (mode d'éditeur), vue `EUW_Graph`, partage de points entre graphes, débits.

## Écrire les graphes à la main (authoring)

Les lieux rêvés ([[Plan_DreamPlaces]]) se font **à la main** : l'outil d'écriture est indispensable. Un essai dans Obsidian a été « horrible » (auteur, 2026-10-05). Pistes, du plus rapide au plus confortable :
- **Un Editor Utility Widget**, dans la lignée du proto `EUW_Graph` : rapide à faire évoluer, en UMG.
- **Un éditeur d'asset avec le framework de graphes d'Unreal** (celui des Blueprints et des matériaux : nœuds, connexions, glisser-déposer, annuler) : le plus confortable, mais il demande un module éditeur. Le plugin open source **GenericGraph** (jinyuliao, sur GitHub) en est un exemple connu, à étudier (`À VÉRIFIER` : sa licence et sa compatibilité avec 5.8).

## Points d'attention

- **Copier / coller** : remapper les GUID des nœuds et des arêtes vers les copies (même problème que les anchors).
- **Sauvegarde** : un graphe sérialisable (dans un acteur, un Data Asset, ou une sauvegarde de partie).
- **Algorithmes utiles une fois pour toutes** : plus court chemin, composantes connexes, couper / fusionner, parcours. Ils serviront à tous les projets.

## Questions ouvertes

- Quels projets, et ce que chacun attend du graphe (spatial ou non, modifiable en jeu, simulation de flux) ?
- ~~Un plugin à part ?~~ → **oui, un plugin à part, réutilisable** ; **commencer par le strict minimum** (auteur, 2026-10-05).
