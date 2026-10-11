# Plan — Tranche verticale

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_3C]], [[Plan_NPC]], [[Plan_VitalPotential]], [[Plan_EireneNexus]], [[Plan_Loot]]

> Statut : 📐 **brouillon** (2026-10-04), à construire avec l'auteur. Rien n'est décidé.

## Pourquoi

Un petit morceau **jouable** qui prouve que le jeu est amusant, et qui force à **choisir** : ce qui est vital maintenant, ce qui peut attendre. Contrainte : un développeur seul, les week-ends et les vacances.

## Ce que la tranche doit prouver (auteur, 2026-10-04)

- **Le monde**, **le côté RPG**, **la défaite sans mort** (« a priori »).
- **La main n'a pas à être prouvée** : l'auteur l'a déjà essayée dans son prototype, elle est amusante. La tranche **s'en sert**.
- **Le loot fait partie du cœur du jeu** : il est dans la tranche.

## La scène centrale (validée comme approche, 2026-10-04)

La tranche est construite autour **d'une scène** qui met en jeu le monde, le choix RPG et la défaite sans mort. Proposition de départ, d'après l'exemple d'*Elden Ring* de l'auteur ; **le boss peut changer** (l'auteur fera peut-être un autre boss).

1. **Un chef exige** du joueur qu'il se soumette (s'agenouiller, un emote lu comme un signal).
2. **Se soumettre** : rejoindre, payer un tribut, gagner son respect, ou tomber dans un piège.
3. **Refuser** : un défi, le combat commence.
4. **Perdre** : un événement de mort imminente (capturé et réduit en esclavage au camp, ou sauvé par des gardes rivaux, avec une dette) ; le prix : argent, loot ou one-up.
5. **Gagner** : le potentiel vital du chef est absorbé, l'arme change ; le groupe sans chef réagit (dispersion, soumission, un rival prend la place).
6. **Revenir plus tard** : le monde se souvient.

Approche : c'est un **prototype de la boucle** (formes grises, pas de finition) centré sur une scène, plutôt qu'une tranche verticale au sens strict (qualité finale). On prouve d'abord ce qui est incertain.

## Contenu (proposition, volontairement minuscule)

| Domaine | Dans la tranche | Hors de la tranche (plus tard) |
|---|---|---|
| **Lieu** | Un coin de l'acte 1 sur PatchTerrain : une prairie, un petit lac (Watershed), un camp ennemi | Le reste de l'acte, les donjons, le feu |
| **Personnage** | Vue subjective, main contrôlée, souris + clavier | Manette, vue épaule |
| **Déplacements** | Marcher, courir, sauter, coyote time + tampon de saut, lever le pied | Glissade, escalade, enjamber, nage |
| **Stances** | idle, attack, defence, parade | interact, emotes, gestes et formes |
| **Arme** | Une seule (`À DÉCIDER` : épée ? lance ?) | Les autres, l'archétype comme classe |
| **Ennemis** | Un type (pièces rigides sur le mannequin), State Tree existant ; peut-être un élite | Les autres, les quadrupèdes |
| **Défaite** | Un événement de mort imminente (ex. KO puis sauvé par des gardes) ; le prix : argent, loot ou one-up | La variété des événements |
| **Potentiel vital** | Version minimale, cachée : tuer nourrit l'arme et le joueur | L'économie par acte, la ponction |
| **Factions** | Deux groupes (bandits, gardes) et une relation qui bouge (la dette) | Eiréné Nexus complet |
| **Loot** | **Fait partie du cœur du jeu** (auteur, 2026-10-04). Version minimale `À DÉFINIR` avec l'auteur | `À DÉFINIR` |
| **Rendu** | Éclairage maison, post-process, formes provisoires | Direction artistique finale |
| **Outils** | L'outil visuel de filtrage d'entrée (indispensable pour régler la main) | Les autres |

## Étapes (chacune jouable)

1. **La main** : outil de filtrage + main contrôlée dans une map de test ([[Plan_3C]], étapes 1 à 3).
2. **Un adversaire immobile** : frapper, parer, rebondir, glisser. Juger le ressenti.
3. **Un adversaire qui se bat** : l'IA (State Tree) et ses phases d'attaque lisibles.
4. **Le lieu** : la prairie, le lac, le camp ; les déplacements minimaux ; la navigation (Navigation Invokers).
5. **La défaite** : l'événement de mort imminente et son prix.
6. **Les conséquences** : potentiel vital minimal, la dette envers les gardes.

## Critères de réussite (proposition)

- Dix minutes de jeu qu'un testeur a envie de refaire.
- Le combat se comprend sans tutoriel écrit.
- La défaite donne envie de continuer plutôt que de recharger.

## Questions pour l'auteur

1. Des trois choses à prouver, laquelle compte le plus ?
2. Quelle arme pour la tranche ?
3. Le lieu proposé te convient, ou tu as une scène en tête ?
4. Qu'est-ce que tu retires, ou ajoutes ?
