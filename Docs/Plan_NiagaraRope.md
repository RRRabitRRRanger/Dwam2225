# Plan — Corde dans Niagara (GPU)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_GpuTraces]], [[Plan_PatchTerrain]]

> Statut : **validé** (2026-10-03). ~~GPU dans Niagara d'abord~~ → **variante CPU codée d'abord** (étape 5 du plan, même algorithme), voir « Décision 2026-10-03 » en bas. Plugin `Rope`, guide [[Guide_Rope]]. Solveur testé (tests automatiques), pas encore vu dans l'éditeur. Vérifié dans le source et le contenu du moteur 5.8 (2026-10-02).

## Objectif

**Objectif précisé par l'auteur (2026-10-02)** : grossièrement **faire le tour d'un objet** avec la corde (l'enrouler autour).

Des cordes, câbles, lianes, sangles et bandoulières simulés sur le GPU, réactifs au personnage et au décor, en éditeur comme en build packagé.

L'auteur cite une technique de Naughty Dog comme inspiration et la jugeait peu accessible. Claude ne connaît pas les détails de cette technique avec certitude (`À CONFIRMER` : jeu, conférence, contenu) et ne l'invente pas. Ce plan part des méthodes standard (PBD), qui suffisent pour enrouler une corde.

## Principe : une chaîne de particules (PBD)

- **Un émetteur GPU**, N particules créées d'un coup : la particule d'indice i est le i-ème nœud de la corde.
- **Chaque frame** :
  1. intégration de Verlet (gravité, vent, inertie) ;
  2. **contraintes de distance** entre nœuds voisins, répétées K fois (*Position Based Dynamics*) : chaque segment reprend sa longueur ;
  3. **épingles** : certains nœuds suivent un point imposé (main, socket, os, point d'accroche) ;
  4. **collisions** : on repousse les nœuds hors des obstacles.
- **Écritures sans conflit** : les contraintes sont résolues en deux passes alternées (segments pairs, puis impairs) dans des *simulation stages*. Deux fils GPU ne modifient donc jamais le même nœud en même temps.
- **Rendu** : un ruban trié par indice (Ribbon Renderer), ou un mesh par segment pour les cordes épaisses.

## Enrouler la corde autour d'un objet

- **Placement initial** : les nœuds sont posés en hélice autour de la boîte englobante de l'objet (rayon un peu plus grand que l'objet, nombre de tours réglable).
- **Serrage** : la longueur de repos des segments diminue progressivement ; la corde se resserre et vient épouser l'objet, retenue par la collision.
- **Collision avec l'objet** : son distance field (GDF), ou une capsule / sphère analytique si l'objet est simple (plus précis).
- **Extrémités** : épinglées (nœud ou point fixe), ou libres et pendantes.
- **Friction** : une friction tangentielle au contact empêche la corde de glisser de l'objet.

## Briques existantes dans Niagara 5.8

| Brique | Usage |
|---|---|
| Module `Constraints/CalculateLinkConstraint` | Contrainte de lien avant/arrière moyennée, avec une vitesse de convergence : la base d'une chaîne |
| Module `Collision/PBD_IntraParticleCollision` | Collisions entre particules (la corde contre elle-même ou contre d'autres cordes) |
| Data interface **Particle Read** | Lire la position d'un autre nœud par indice ou par identifiant (`Get Position By Index`…) |
| Data interface **Collision Query** | `QueryMeshDistanceFieldGPU` : distance et normale dans le Global Distance Field |
| Modèle `BehaviorExamples/PendulumConstraint` | Exemple de contrainte à étudier |

## Collisions : qui voit quoi

| Obstacle | Méthode |
|---|---|
| Décor statique avec MDF (et POI cuits) | Global Distance Field |
| **Terrain PatchTerrain** | **Pas dans le GDF.** Il faut la texture de hauteur du terrain (prévue dans [[Plan_PatchTerrain]]) ou des traces HW Ray Tracing ([[Plan_GpuTraces]]) |
| Personnage | Sphères et capsules passées en paramètres (sur les os principaux) : simple, précis et bon marché |

## Gameplay ou simple visuel ?

- **Visuel** (lianes, câbles, sangles, bandoulières) : 100 % Niagara, aucun retour au CPU.
- **Gameplay** (s'y suspendre, tirer un objet) : la latence GPU → CPU gêne. Pour ces cas, une version CPU en C++ (même algorithme, peu de nœuds), ou le plugin moteur **CableComponent** (Verlet sur CPU) pour démarrer.

## Répartition du travail

| Qui | Quoi |
|---|---|
| **Claude** | Le HLSL des étapes de simulation (Verlet, contraintes paires/impaires, épingles, collisions), versionné en texte ; un petit composant C++ qui transmet les points d'accroche et les capsules du personnage au système Niagara ; un pas-à-pas de montage |
| **Auteur** | Le système Niagara (émetteur, simulation stages, renderer) en suivant le pas-à-pas ; les tests |

## Étapes proposées

1. Corde suspendue entre deux points fixes (Verlet + contraintes), rendue en ruban.
2. Épingles dynamiques (accrochée à un os, à une main).
3. Collisions : capsules du personnage, puis GDF, puis terrain.
4. Bandoulière / ceinture : une boucle fermée autour du torse.
5. Si besoin : version CPU pour les cordes de gameplay.

## Questions ouvertes

- Premier cas d'usage : liane du décor, ou bandoulière du personnage ?
- Nombre de cordes simultanées visées.
- La technique de Naughty Dog : `À CONFIRMER`.

## Décision 2026-10-03 : CPU d'abord

- **Contrainte trouvée dans le moteur** : le *Particle Attribute Reader* d'un émetteur sur lui-même renvoie les données **de la frame précédente** (`NiagaraDataInterfaceParticleRead.cpp`, description de `Get Position By Index`). Les itérations de contraintes d'une frame ne voient donc pas les corrections des voisins. Un solveur GPU devra passer par une grille (*Grid 2D Collection*, lue et écrite à chaque *simulation stage*).
- **Décision** : la variante CPU de l'étape 5 est faite d'abord (plugin `Rope`, composant `URopeComponent`). Elle couvre l'objectif « faire le tour d'un objet », se teste hors du jeu et ne demande aucun montage Niagara. Le rendu Niagara (ruban) reste possible via `Get Node Positions`.
- **Ajouts** par rapport au plan : auto-collision (sinon les tours d'un enroulement se tassent les uns dans les autres), frottement de position, bouts attachés à l'obstacle lors d'un enroulement. Les tests ont montré qu'une corde serrée à bouts libres se déroule et qu'une corde lâche glisse le long d'un poteau vertical (comportement physique).
- **Reste à faire** : version GPU (Grid2D) si des dizaines de cordes décoratives sont nécessaires ; collisions avec le terrain par sa texture de hauteur (le balayage du décor voit déjà sa collision).
- La technique de Naughty Dog : toujours `À CONFIRMER`.
