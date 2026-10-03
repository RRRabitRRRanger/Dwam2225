# Guide — Watershed : eau par bassins

> Accueil : [[00 - Accueil]] · Plan : [[Plan_Water]] · Lié à : [[Plan_PatchTerrain]]

> État (2026-10-03) : plugin `Watershed` compilé. **Algorithme testé** hors jeu par 5 tests automatiques (cas calculés à la main + grand terrain bruité). **Pas encore testé dans une map.**

## Ce qui est fait

| Étape du plan | État |
|---|---|
| 1. Hiérarchie 2,5D sur CPU, debug visuel | ✅ Codée et testée (tests automatiques) |
| 2. Fill–Spill–Merge, sources, rendu simple des lacs | ✅ Codé ; rendu en tuiles (quadtree implicite) |
| 3. Mises à jour locales asynchrones | 🔧 Partiel : rééchantillonnage local + reconstruction **globale** en tâche de fond (assez rapide, voir les mesures) |
| 4. Volumes 3D (cavernes), air piégé | Pas commencé |
| 5. Simulation locale GPU (tuyaux virtuels), eau en mouvement | Pas commencé |

## Mesures (test `ConservationLarge`, CPU de l'auteur)

Terrain bruité de 512 × 512 cellules (1 km × 1 km en cellules de 2 m) : 41 163 creux, 54 948 nœuds.

- Construction de la hiérarchie : **84 ms** (en tâche de fond en jeu).
- 50 mm de pluie sur toute la région : **8 ms**, 11 662 lacs.
- Niveaux de tous les lacs : **2 ms**.
- Conservation : lacs + eau sortie = eau versée, erreur nulle.

Non mesuré : les **traces de hauteur** (une par cellule, sur le thread de jeu). Elles coûteront sans doute plus que la construction sur une grande région : à mesurer (le log `LogWatershed` affiche le temps).

## Mise en place

1. Placer un **Watershed Volume** (*Place Actors*, chercher « Watershed »).
2. Étirer sa boîte (*Bounds*) : elle doit couvrir le terrain en X / Y, et **dépasser au-dessus et en dessous** en Z (les traces partent du haut de la boîte et descendent jusqu'au bas).
3. Cliquer **Rebuild** (panneau Détails). Le log affiche le nombre de cellules, de creux, et les temps.
4. Cocher **Debug Draw** : sphères vertes = cols entre bassins, rouges = cols vers l'extérieur ; texte cyan = lacs (niveau, volume / capacité). *Debug Draw Heights* montre les hauteurs échantillonnées.
5. Cliquer **Add Test Rain** (100 mm) : les lacs apparaissent. **Clear Water** vide tout.
6. *Water Material* : un matériau d'eau (voir plus bas).

Le terrain **PatchTerrain** génère sa collision : les traces le voient. N'importe quel mesh avec collision convient (*Trace Complex* coché par défaut).

## Réglages

| Réglage | Rôle |
|---|---|
| *Cell Size* | 2 m par défaut et minimum (décision du plan) |
| *Edges Drain* | Les bords de la boîte évacuent l'eau. Décoché : cuve fermée |
| *Trace Channel*, *Trace Complex*, *Ignored Actors* | Ce que les traces voient |
| *Max Cells* | Garde-fou (traces sur le thread de jeu) : 262 144 = 512 × 512 |
| *Sources* | Points (déplaçables dans la vue, losanges) avec un débit en m³/s, actifs en jeu |
| *Initial Rain* | Pluie versée au début du jeu (mm) |
| *Max Tile Level* | Plus grande tuile : 2^N cellules (5 → 64 m) |
| *Min Depth* | Sous cette profondeur, une cellule est sèche |
| *Tile Update Interval* | Les tuiles sont reconstruites au plus toutes les N s quand l'eau bouge |

Une cellule sans sol sous elle (trace qui ne touche rien) est un **trou** : l'eau s'en va par là.

## Blueprint

- **Rebuild** : rééchantillonne et reconstruit. L'eau déjà versée est conservée et redistribuée.
- **Rebuild Region** (boîte monde) : rééchantillonne seulement cette zone, puis reconstruit. À appeler quand un obstacle apparaît ou que le terrain change.
- **Add Water At Location** (position, m³) : renvoie le volume parti vers l'extérieur.
- **Add Rain** (mm), **Clear Water**.
- **Get Water At Location** : niveau et profondeur (pour nager, flotter, éteindre une torche…).
- **Get Num Wet Lakes**, événement **On Hierarchy Built**.

## Matériau d'eau

Les tuiles sont des plans carrés à la hauteur de leur lac. Le rivage est **par excès** : les cellules sèches qui bordent un lac reçoivent aussi une tuile, mais leur sol est plus haut que l'eau et la cache (test de profondeur). Le rivage est donc net au pixel, sans escalier.

- Un matériau **Unlit** (pipeline maison) ou **Translucent**.
- *Per Instance Custom Data* : indice 0 = **niveau** du lac (cm), indice 1 = **numéro** du lac.
- Pour un rivage doux : en translucide, *Depth Fade* ou (`SceneDepth` − `PixelDepth`) comme opacité.
- Les tuiles ne projettent pas d'ombre et n'ont pas de collision.

## Limites connues

- **2,5D** : un seul niveau de sol par cellule. Une arche ou une caverne est vue comme pleine (étape 4 du plan).
- **Traces sur le thread de jeu** : un gros *Rebuild* peut figer une frame. *Rebuild Region* limite le coût.
- **Reconstruction globale** : même pour une mise à jour locale, toute la hiérarchie est reconstruite (84 ms pour 1 km², en tâche de fond en jeu). Une vraie reconstruction locale ne vaudra la peine que si ce coût gêne.
- **Eau statique** : niveaux de lacs au repos, pas de courant (étape 5).
- Les sources accumulent l'eau versée : un lac plein déborde, rien ne s'évapore.

## Algorithme (résumé)

Voir [[Plan_Water]]. Barnes, Callaghan & Wickert (2021), *Fill–Spill–Merge* : creux → remplissage par priorité (étiquettes + cols) → arbre des fusions (union-find, cols du plus bas au plus haut). Volume d'un bassin au niveau h = somme des (h − z) sur ses cellules plus basses que h ; le niveau se retrouve par dichotomie. Code : `Plugins/Watershed/Source/Watershed/Private/WatershedHierarchy.cpp`.
