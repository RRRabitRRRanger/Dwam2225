# Plan — Arbres procéduraux et dynamiques

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Fire]], [[Plan_WorldState]], [[Plan_VitalPotential]]
> Source : le doc de l'auteur « Idées d'outils Unreal » (Claude Docs, 2026-10-05).

> Statut : 💭 **idée**. **Système Unreal, pas strictement DWAM.** Rien n'est codé.

## L'idée (auteur)

Un arbre est un **graphe** qui pousse par *Space Colonization*, se casse en temps réel et repousse.

1. **Structure de données** : un graphe orienté (un arbre mathématique). Chaque nœud a une position locale, une orientation, un rayon, un âge et un type (tronc, branche, bourgeon). Le graphe parent stocke la topologie ; le même graphe peut passer par différentes fonctions de rendu (bruit sur les positions, multiplicateur de rayon).
2. **Croissance** : *Space Colonization*. Un nuage de points 3D (des attracteurs : nutriments et lumière). Les bourgeons calculent la direction moyenne vers les attracteurs à portée, créent un nœud et « consomment » l'attracteur. **La timidité des cimes** (*crown shyness*) vient toute seule : des arbres voisins partagent les mêmes attracteurs, le premier arrivé les consomme.
3. **Mesh** : extrusion le long des segments (cylindres, capsules) ; feuillage en instances (quads ou cartes) sur les bourgeons ; un même graphe donne plusieurs meshes selon le profil d'extrusion.
4. **Destruction en temps réel** (façon *Far Cry*) : collision ou trace sur les capsules des segments ; le graphe est coupé au nœud touché ; la partie haute devient un corps physique qui tombe, la partie basse reste, amputée.
5. **Repousse** : le nœud coupé devient un bourgeon actif ; un petit nuage d'attracteurs apparaît autour de la plaie ; la croissance reprend, la longueur et le rayon des nouvelles branches grandissent avec le temps.

## Remarques (Claude)

- **Référence** : Runions, Lane & Prusinkiewicz (2007), *Modeling Trees with a Space Colonization Algorithm*.
- **Les liens possibles**, si un jour ça sert dans DWAM :
  - **le feu** ([[Plan_Fire]]) : un arbre en graphe, ce sont déjà des nœuds de combustible ;
  - **le monde qui guérit** ([[Plan_WorldState]]) : la repousse est dans l'idée même ;
  - **le potentiel vital** ([[Plan_VitalPotential]]) : la végétation est l'un des réservoirs de ton prototype.
- **Les attracteurs** pourraient venir du terrain et de l'eau (plus près d'un lac, plus d'attracteurs).

## Un graphe, plusieurs arbres (auteur, 2026-10-05)

L'auteur veut des arbres (le feuillage) où **un graphe donne plusieurs arbres crédibles**, même construits en kit : plus de variation que l'habituel « arbre en kit, tourné au hasard ». « C'est une bonne piste, même si le construire n'est peut-être pas si simple. » Même principe que les lieux rêvés ([[Plan_DreamPlaces]]) : le graphe est l'identité, la mise en espace varie.

## Questions ouvertes

- Combien d'arbres à l'écran, et à quelle distance doivent-ils rester dynamiques ?
- La croissance en jeu, ou seulement à la génération (et à la repousse) ?
