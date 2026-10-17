# Plan — Rythme : horloge musicale et événements sur le beat

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Fire]], [[Plan_Lights]], [[Plan_PostProcess]]

> Statut : 💭 **idée**. Source : le doc de l'auteur « Plan de jeu UE5 - Systèmes et Features » (Claude Docs, 2026-10-16), brique C et foudre. Rien n'est décidé, rien n'est codé.

## Intention (doc de l'auteur, 2026-10-16)

- **Une horloge musicale centrale** et un **scheduling prédictif** pour que des événements du monde tombent sur le beat.
- **Back-calcul du temps de trajet** : on lance l'événement en avance, pour qu'il arrive sur le beat.
- Exemple : **le tonnerre tombe sur le beat**. On tire l'éclair en avance de distance / vitesse du son.
- Partage logique / visuel : système autonome et **très testable** (on mesure l'écart entre l'arrivée du son et le beat).

## La foudre : une feature carrefour (doc de l'auteur, 2026-10-16)

La foudre croise trois systèmes : le rythme (ce plan), le feu ([[Plan_Fire]]) et la lumière ([[Plan_Lights]]).

- **Logique** : le timing, l'ignition (la foudre met le feu) et les dégâts.
- **Visuel** : le flash et les VFX.

## Place dans l'ordre de dé-risquage (doc de l'auteur)

Troisième, après la brique A (attache sur un mesh animé) et le feu, parce qu'elle est autonome et testable. Cet ordre n'est pas encore confronté à [[Plan_VerticalSlice]].

## Précisions de l'auteur (2026-10-17)

- **Un « tempo subsystem »**, ou équivalent : une source de tempo unique pour tout le jeu.
- **N'importe quoi doit pouvoir tomber sur le beat** : la musique dynamique, les coups, les effets de particules, les VFX.
- Rappel de l'auteur : le doc du 2026-10-16 était un **dump de souhaits**, pas une conception.

## Questions ouvertes

- Qui mène : la musique jouée donne le temps (lecture de la position de la piste), ou l'horloge pilote la musique dynamique ? `À COMPLÉTER`
