# Guide — Corde (plugin Rope)

> Accueil : [[00 - Accueil]] · Plan : [[Plan_NiagaraRope]] · Lié à : [[Plan_GpuTraces]]

> État (2026-10-03) : plugin `Rope` compilé. **Solveur testé** hors jeu par 4 tests automatiques. **Pas encore vu dans l'éditeur** : le rendu du tube et les accroches restent à vérifier.

## Pourquoi sur CPU (et pas encore dans Niagara)

Le plan visait d'abord Niagara (GPU). En vérifiant le source 5.8 : un émetteur Niagara qui lit ses propres particules (*Particle Attribute Reader* sur lui-même) reçoit **les positions de la frame précédente** (description de `Get Position By Index`). Les itérations de contraintes d'une même frame ne verraient donc pas les corrections des voisins. La version GPU demande une grille intermédiaire (*Grid 2D Collection*) et un montage Niagara délicat, impossible à tester sans l'éditeur.

L'étape 5 du plan prévoyait déjà une version CPU (gameplay, peu de nœuds). C'est elle qui est faite, avec le même algorithme. Elle couvre l'objectif « faire le tour d'un objet ». La version Niagara reste possible plus tard pour des dizaines de lianes décoratives.

## Ce que fait le composant **Rope**

- Verlet + contraintes de distance (PBD), sous-pas et itérations réglables.
- **Accroches** : chaque bout est libre, ou attaché à un point du composant, à un acteur, ou à un socket ou un os (main, ceinture).
- **Obstacles analytiques** (*Colliders*) : sphère, capsule, boîte, éventuellement attachés à un acteur ou à un os ; ou capsule ajustée aux bornes d'un acteur.
- **Décor** : balayage d'une sphère par nœud (canal *Visibility* par défaut).
- **Auto-collision** : les tours d'un enroulement restent côte à côte.
- **Frottement** au contact.
- **Enroulement** : `Wrap Around` (hélice + bouts attachés), puis `Tighten To` pour serrer.
- **Rendu** : le composant est lui-même un *Dynamic Mesh* (tube). Choisir un matériau, c'est tout.

## Mesures (tests automatiques)

| Test | Résultat |
|---|---|
| Corde de 3 m entre deux points à 2 m | Étirement 0,18 %, flèche 1 m |
| Corde lâchée sur une barre | Aucune pénétration, posée pile sur la barre, les bouts pendent |
| Pente de 20°, frottement 0,8 / 0 | Glisse de 1,4 cm / 349 cm en 2 s |
| 3 tours autour d'un poteau, bouts attachés, serrée à 97 % | Reste plaquée (rayon 22,3 cm pour 20 + 2), tendue de 2,3 %, stable (0,5 cm/s) |

Coût, d'après les horodatages des tests : environ **0,2 ms par sous-pas** pour 61 nœuds avec auto-collision et 32 itérations, **0,04 ms** pour 31 nœuds sans auto-collision. Avec 2 sous-pas, une corde enroulée coûte donc environ 0,4 ms par frame. À mesurer en jeu.

## Mise en place

### Corde qui pend

1. Sur un acteur (ou un Blueprint) : *Add Component* → **Rope**.
2. *Start Anchor* : *Attached* coché (par défaut), sans acteur → le bout suit le composant.
3. *End Anchor* : décoché → le bout pend ; coché → *Offset* (espace du composant) ou un acteur / socket.
4. *Length*, *Num Segments* (32), *Thickness* (2 cm), un matériau (*Materials* → élément 0).
5. Jouer. Pour un aperçu sans jouer : **Simulate In Editor**.

### Bandoulière ou corde tenue en main

- *Start Anchor* → *Actor* = le personnage, *Socket* = `hand_r` (ou un socket du squelette).
- Les membres du personnage : des *Colliders* capsule attachés aux os (*Actor* = le personnage, *Socket* = `spine_03`, `thigh_l`…). Plus précis et moins cher que la collision du décor.
- Les acteurs des accroches sont ignorés par le balayage du décor.

### Enrouler autour d'un objet

1. *Colliders* → un élément : *Shape* = **Capsule**, *Actor* = l'objet, puis soit **Fit To Actor Bounds**, soit *Radius* / *Half Height* / *Offset* à la main. Pour un objet couché, tourner l'*Offset* (l'axe de la capsule est son Z local).
2. *Num Segments* : **64** pour 3 tours (environ 20 nœuds par tour : moins, et le tube coupe un peu l'objet entre deux nœuds).
3. Bouton **Wrap Around First Collider** (ou `Wrap Around` en Blueprint, puis `Tighten To`) : 3 tours, bouts attachés à l'objet, serrage à 97 %.
4. Si l'objet bouge, les bouts le suivent et la corde suit.

À savoir (vérifié par les tests) : **sans bouts attachés**, une corde serrée se déroule, et une corde lâche glisse le long d'un poteau vertical. C'est la physique, pas un bug. Il faut l'attacher, ou qu'elle soit tendue.

## Réglages utiles

| Réglage | Effet |
|---|---|
| *Iterations* (16) | Plus = moins élastique sous tension. La tension d'un enroulement serré étire d'environ 2 % à 32 itérations |
| *Sub Steps* (2) | Plus = plus stable aux vitesses élevées (coût proportionnel) |
| *Friction* (0,6) | 0 = glisse, 1 = colle |
| *Damping* (0,01) | Calme les oscillations |
| *Stiffness* (1) | < 1 : corde élastique |
| *Tighten Speed* | Vitesse de serrage (cm/s) |
| *Self Collision* | Coût en N² : garder ≤ 64 à 128 nœuds |
| *Collide With World*, *Collision Channel*, *Ignore Owner* | Décor |

## Rendu Niagara (optionnel)

*Generate Mesh* décoché : seulement la simulation. En Blueprint, chaque frame : **Get Node Positions** → **Set Niagara Array Position** (paramètre utilisateur d'un système Niagara) ; dans Niagara, un émetteur de N particules dont la position est lue dans le tableau par indice, avec un **Ribbon Renderer**.

## À vérifier dans l'éditeur

- **Le tube** : il est construit avec la même règle d'enroulement des triangles que PatchTerrain. S'il apparaît retourné (on voit l'intérieur), cocher *Two Sided* sur le matériau et me le signaler.
- Les accroches à des sockets ou des os.
- Le balayage du décor sur le terrain PatchTerrain (il a une collision).

## Retours du premier test dans l'éditeur (auteur, 2026-10-12)

Test : une sphère avec un composant Rope, secouée à la main, près d'un poteau et d'un mur.

- **Ça marche** : la corde pend, suit la sphère, se pose au sol et s'enroule « assez bien » autour du poteau à la longueur par défaut.
- **Léger jitter** au repos.
- **Longueur doublée : l'enroulement ne tient plus**, la corde reste accrochée par quelques points seulement et devient anguleuse. Lecture de Claude (à confirmer) : *Num Segments* n'a pas changé, donc chaque segment est deux fois plus long ; or la collision se fait par nœud, pas par segment, et un segment long coupe à travers l'obstacle. À essayer : doubler aussi *Num Segments*.
- **Un pli du tube** vu une fois (le tube se pince et se vrille sur lui-même, capture de l'auteur), pas reproduit. Piste de Claude : le repère des sections du tube se retourne quand la corde se replie sur elle-même.
- La corde a pu être « un peu cassée » en la secouant fort.

**Demande de l'auteur** : à terme, trouver de « vrais » solveurs et s'aligner sur l'état de l'art plutôt que ce solveur maison. Pistes à étudier (Claude, rien de décidé) : XPBD et les petits pas de temps (Macklin), les tiges de Cosserat en PBD (Kugelstadt), la collision par segment (capsules) au lieu de par nœud, et pour l'enroulement les cordes « à points de contact » des jeux (segments tendus entre des points d'appui ajoutés et retirés).

Le doc de l'auteur « Plan de jeu UE5 » (2026-10-16) nomme la corde « Cosserat » (brique B : sangles, cheveux qu'on tire pour diriger une monture). Lecture validée par l'auteur (2026-10-17) : c'est une **piste pour ce solveur à l'état de l'art**, pas une remise en cause du plugin actuel.
