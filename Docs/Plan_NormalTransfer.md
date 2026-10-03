# Plan — Transfert de normales aux intersections + jouet « snake »

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_GpuTraces]]

> Statut : **validé d'office par l'auteur** (« fais le plan et enchaîne », 2026-10-02). Implémentation en cours.

## Contexte

Les personnages de DWAM sont des meshes rigides attachés aux os, pas des skinned meshes. Pour donner l'illusion d'un corps continu, l'auteur utilise une méthode maison : un « faux data transfer » de normales aux zones de contact, à base de **distance la plus proche** (nœuds `DistanceToNearestSurface` / `DistanceFieldGradient`, présents dans `MaterialBase*`, `DebugShader`…). Le graphe exact n'est pas lisible depuis les `.uasset` ; ce plan repart du principe.

## Deux variantes, une bibliothèque

Fichier : `Plugins/DwamLighting/Shaders/NormalTransfer.ush`, chemin virtuel `/Plugin/DwamLighting/NormalTransfer.ush`, à mettre dans *Include File Paths* d'un nœud Custom.

La bibliothèque est **pure mathématique** : elle n'appelle aucune fonction du moteur. Les entrées dépendantes du moteur (GDF, Custom Primitive Data) sont lues dans le nœud Custom ou par des nœuds standard, puis passées en paramètres. Ça évite tout problème d'ordre d'inclusion.

### A. Variante « champ » (GDF) — tout mesh avec MDF, aucune préparation

- On sonde le Global Distance Field **un peu au-dessus de la surface**, en `P + N × Probe`.
- Sur une surface isolée, la distance trouvée vaut environ `Probe`. Près d'une jonction concave (bras contre torse), une autre surface est plus proche, donc la distance est plus petite.
- Masque = `1 − distance / Probe`, adouci par un smoothstep.
- La normale est mélangée vers le **gradient du GDF** à ce point : c'est la normale de l'*union* des objets, lissée par la résolution du GDF. Les plis disparaissent dans l'éclairage.
- Limites : précision du GDF (~20 cm près de la caméra), uniquement les meshes avec MDF ; le terrain PatchTerrain n'y est pas.
- **Important** : un matériau n'a accès au GDF que s'il contient un nœud GDF (DistanceToNearestSurface ou DistanceFieldGradient). On utilise justement ces nœuds pour lire la distance et le gradient.

### B. Variante « analytique » — primitives connues (sphères, capsules)

- Les voisins sont décrits par des formes simples : sphères ou capsules (centre, rayon), passées par **Custom Primitive Data** (9 float4 disponibles).
- On calcule la distance signée exacte à chaque forme et son gradient, puis on fait une **union lisse** (*smooth min* polynomial d'Inigo Quilez) dont on prend le gradient : la normale de la surface « fusionnée », exacte, sans dépendre du GDF.
- Positions **relatives à l'objet** (pas de coordonnées monde absolues), pour rester précis loin de l'origine.
- Idéal pour les personnages (une capsule par os voisin) et pour le snake.

## Le jouet « snake »

Acteur C++ `ASnakeToy` (module de jeu, c'est un jouet) :

- une file de sphères (mesh `/Engine/BasicShapes/Sphere`), rayon décroissant vers la queue ;
- la tête **rampe sur les surfaces** de la scène : traces CPU complexes (géométrie visuelle). Elle monte sur un mur rencontré, contourne une arête, retombe si elle perd le sol, et erre avec un bruit de direction ;
- le corps suit le **chemin de la tête** (espacement constant le long du trajet) ;
- chaque sphère reçoit dans ses Custom Primitive Data les positions **relatives** et rayons de ses deux voisines, plus le facteur de lissage. Le matériau n'a plus qu'à appeler la variante analytique ;
- visible en éditeur (corps posé en ligne) et animé en jeu.

Disposition des Custom Primitive Data (par sphère) :

| Index (float) | Contenu |
|---|---|
| 0–3 | Voisine précédente : offset X, Y, Z (cm, relatif au centre de la sphère), rayon (0 = aucune) |
| 4–7 | Voisine suivante : offset X, Y, Z, rayon (0 = aucune) |
| 8–11 | Rayon propre, lissage K (cm), libre, libre |

## Ce que l'auteur fait dans l'éditeur

Créer un matériau avec un nœud Custom (code et branchements dans le guide `Guide_NormalTransfer.md`), l'assigner au snake. Sans matériau, le snake fonctionne avec le matériau par défaut, sans le lissage.

## Limite connue de la variante hybride : le « Ground Rejection »

L'actuel `GroundRejection` est une heuristique géométrique : il coupe la fusion quand le pixel regarde vers le bas et que la normale fusionnée pointe vers le haut. Le GDF ne sait pas ce qui est « le sol » : en pratique il rejette **tout ce qui est dessous** (cube posé sur un cube, menton contre poitrine, bras replié). Sur un personnage, le laisser à 0.

Amélioration prévue (dépend de la texture de hauteur du terrain, cf. [[Plan_PatchTerrain]] et [[Plan_Water]]) : comparer la distance du GDF à la hauteur du point sondé au-dessus du terrain. Si elles sont égales, la surface la plus proche est le sol et on rejette ; sinon c'est un autre objet et on fusionne. On obtient un vrai rejet du sol, sans faux positifs.

## Retard du GDF sur les objets mobiles (constaté sur le snake, 2026-10-03)

Avec les variantes qui lisent le GDF (champ, surface, hybride), l'éclairage des jonctions « glisse » sur les objets qui bougent. Cet effet de **péristaltisme / reptation** est acceptable (même joli) sur un serpent, mais gênant ailleurs.

Causes (vérifiées dans `GlobalDistanceField.cpp`, moteur 5.8) :

- **mise à jour étalée** : seule la clipmap 0 (25 m) est recalculée à chaque frame ; les suivantes le sont toutes les 2, 4 puis 8 frames (`r.AOGlobalDistanceFieldStaggeredUpdates = 1`, `r.AOGlobalDistanceFieldClipmapUpdatesPerFrame = 2`) ;
- **décalage d'environ une frame** entre la composition du champ et sa lecture par le matériau ;
- **voxels d'environ 20 cm** : le champ évolue par à-coups quand l'objet traverse les voxels.

Réglages possibles, **à évaluer en fin de projet**, quand le coût et la marge seront connus :

| Réglage | Effet | Coût / risque |
|---|---|---|
| `r.AOGlobalDistanceFieldStaggeredUpdates 0` | Toutes les clipmaps à chaque frame | Mise à jour du GDF plus chère |
| `r.LumenScene.GlobalSDF.ClipmapExtent 1250` | Voxels proches d'environ 10 cm | Zone précise réduite (ombres SDF comprises) |
| `r.LumenScene.GlobalSDF.Resolution` plus haut | Précision partout | Mémoire, coût |

**Attention (auteur)** : ces réglages, justes en théorie, peuvent casser le rendu d'un coup (écran noir, comme une branche sans repli). Ils restent simples à essayer à chaud dans la console : tester un réglage à la fois, en gardant les valeurs d'origine sous la main. Ils touchent aussi les ombres SDF, qui lisent le même champ.

## Étapes

1. Squelette `DwamLighting` (module `PostConfigInit`, mapping `/Plugin/DwamLighting`) + `NormalTransfer.ush`.
2. `ASnakeToy`.
3. Guide de montage des matériaux (variante champ et variante analytique).
4. Plus tard : capsules par os pour les personnages ; outil qui remplit les Custom Primitive Data depuis un squelette.
