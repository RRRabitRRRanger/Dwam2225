# Checklist de l'auteur

> Accueil : [[00 - Accueil]] · Feuille de route : [[Feuille_de_route]]

> Tenue par Claude, comme **trace** : les questions sont posées à l'auteur **dans le chat** (sa préférence, 2026-10-25), puis les réponses sont reportées ici.

## A. Décisions, une fois pour toutes (bloquent la nouvelle façon de travailler)

- [x] **A2. Commits** → Claude **commite sans demander** (on peut revenir en arrière), **sur la branche courante**, pas de branche à lui (auteur, 2026-10-25).
- [x] **A3. Lancer l'éditeur** → **toujours avec sa fenêtre**, pour que l'auteur sache ce qui tourne et ne fasse pas d'erreur en parallèle (auteur, 2026-10-25).
- [x] **A1. Assets de test générés** → **oui, dans `Content/_Claude/` seulement**, par script Python d'éditeur (« excellente idée ») ; Claude dit à chaque fois ce qu'il a créé, pour tester quoi et comment (auteur, 2026-10-25). Exception à la règle « jamais de `.uasset` » : Claude peut **générer par script Python d'éditeur** des assets de test dans `Content/_Claude/` seulement (préréglages, matériaux de test, Blueprints de démo), jamais à la main, jamais ailleurs ; dossier effaçable à tout moment. Oui / non ?
- [ ] **A4. La feuille de route** : les paliers P1 / P2 / P3 et l'ordre J0 → J5 te conviennent ? Ce qui manque, ce qui est en trop ?
- [ ] **A5. La taille de la 1.0** : combien d'actes, combien d'heures de jeu visées ?

## B. Décisions de design (avant le jalon concerné)

- [ ] **B1 (J2).** L'arme de la tranche : épée, lance, autre ? ([[Plan_VerticalSlice]])
- [ ] **B2 (J2).** Ce que fait chaque stance avec la main et la caméra (en défense : caméra verrouillée sur la cible ?) ([[Plan_3C]])
- [ ] **B3 (J1).** Valider [[Plan_EditorUX]] (convention de debug des plugins) ; A ou B pour ranger le code d'éditeur.
- [ ] **B4 (J5).** Le loot minimal de la tranche. ([[Plan_Loot]])
- [ ] **B5 (J5).** Où va la perte quand on tue : à la nature ? ([[Plan_VitalPotential]])
- [ ] **B6 (J1).** Le plugin `UTCWS` (armes modulaires, marques, périodes) : on s'en sert comme base du loot, ou c'est un essai à mettre de côté ? Le plugin `VerbsSystem` (vide) : à garder ?

## C. Recettes en attente (déjà codé, jamais vu)

Classées par utilité pour P1. Quand J0 sera fait, Claude en reprendra la plupart lui-même ; restera seulement ce qui demande ton œil.

- [ ] **C1. CharacterKit M1** : pièces qui suivent le mannequin ([[Guide_CharacterKit]]). → reprise par Claude à J0.
- [ ] **C5. SDFEyes U2** : matériau de test ([[Guide_SDFEyes]]). → remplacé par M3 (component + asset de forme), plus de câblage à la main.
- [x] Post-process, diffus physique, transfert de normales, PatchTerrain, GPUTrace (GPU), LayoutKit, GraphKit : **vus par l'auteur** (2026-10-25).

## D. Recettes de goût (seul l'auteur peut juger)

Ajoutées au fil des lots.

- (vide pour l'instant)
