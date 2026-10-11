# Plan — Outil ville : BP Anchor et édition façon Blender

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_KitbashTool]], [[Plan_PatchTerrain]]
> Source : le doc de l'auteur « Idées d'outils Unreal » (Claude Docs, 2026-10-05), onglet « Outil ville : splines et BP Anchor ».

> Statut : 💭 **idée**. **Outil Unreal, pas strictement DWAM.** Rien n'est codé.

## Le problème (auteur)

Dans les outils pour faire une ville, les splines d'Unreal sont toujours horribles et pénibles à manipuler.

## L'idée : des BP Anchor (auteur)

Remplacer la spline par des Blueprints **Anchor**, chacun avec une forme qui sert de poignée.
- **Orientation** : chaque anchor peut regarder le suivant, ce qui permet d'« animer » la spline.
- **Géométrie** : gérer les lissages, mais aussi les **embranchements**.
- **Partage** : avec des tags, des points peuvent être partagés entre plusieurs systèmes.
- **Copier / coller** : le problème qui en découle, réglé par **GUID / soft reference**.
- **Sol** : la forme qui fait un sol, avec « l'origine flottante » début / milieu / fin ; ce sont juste des BP parentés.

## Édition façon Blender (auteur)

L'édition doit marcher comme dans Blender, parce que dans Unreal c'est pénible :
- éditer en position monde, en position locale, etc. ;
- montrer les positions possibles ;
- *apply* de l'emplacement, de la rotation et de l'échelle, comme dans Blender.

## L'existant

- **`Plugins/CitySplineTools`** (« outils pour faire des villes, en commençant par les routes »), fichiers datés au plus tard de janvier 2026, oublié par l'auteur. Contenu réel (lu par les noms) :
  - `BP_SplineBase_CST` : un acteur Dynamic Mesh avec une spline. Il échantillonne des points le long de la spline, **triangule le contour** (Delaunay), extrude, ajoute du bruit de Perlin, lisse, recalcule les normales. C'est plutôt une **surface remplie à partir d'un contour** qu'une route : ni voies, ni largeur, ni intersections.
  - `BP_ProceduralMesh` : un test de grille (rectangle subdivisé).
  - `CST_SplinePinner` : un acteur presque vide (une ébauche).
  - `CitySocket` : un mesh statique servant de repère.
  - Map `CST_InDev` : deux `BP_SplineBase_CST` posés, l'éclairage par défaut.
  - **Décision de l'auteur (2026-10-05)** : à garder **à part**, voire **à virer**. Pas une base pour l'outil ville.
- **`Content/UI/EUW_Graph`** (mai 2026) : un proto d'éditeur de graphes (« pour la chaufferie, les routes… »). **Aucun lien** avec `CitySplineTools` (vérifié : aucun ne référence l'autre). L'auteur le trouve intéressant (« EUW graph c'est cool »).

## Remarques (Claude)

- **Des anchors reliés, avec des embranchements, c'est un graphe** (des nœuds et des arêtes). `EUW_Graph` pourrait devenir la **vue en graphe** des mêmes données : le réseau de routes vu de haut, en nœuds.
- **Copier / coller** : une soft reference copiée pointe encore vers l'original. Au collage, il faut **remapper** les références vers les copies (via les GUID) : c'est le vrai travail derrière « réglé par GUID ».
- **Édition façon Blender dans Unreal** : possible avec un **mode d'éditeur** en C++ (le framework d'outils interactifs, celui des Modeling Tools). Ça demande un module éditeur, distinct du runtime.
- **« Apply transform »** : reporter la transformation de l'acteur dans ses enfants (ou dans les positions des anchors), puis remettre l'acteur à l'identité.

## Paramétrique à toutes les échelles (auteur, 2026-10-05)

- **Emboîté** : une **maison** paramétrique, dans un **quartier** paramétrique, peut-être dans une **ville** paramétrique. Chaque niveau est un graphe avec ses règles de mise en page ([[Plan_KitbashTool]]).
- **Les routes** (intersections, carrefours, bretelles) : « chiant, mais je commence à choper le truc ».
- **Simplification plutôt que jumeau numérique** : en paramétrique, l'avantage est de pouvoir **simplifier**, pas de reproduire fidèlement le réel.
- **Pas un open world**, « mais quand même ».

### Lecture (Claude)

- ~~**Les routes dans le graphe** deviennent plus simples~~ → **corrigé par l'auteur** (expérience personnelle) : ce n'est pas plus simple, c'est de la **topologie**. Ça dépend du nombre de branches, de leurs **angles d'arrivée** ; on ne s'arrête pas à la continuité C1, il faut souvent du **G2**, avec des **clothoïdes** ; une **bretelle** n'a rien à voir avec un carrefour ; il y a plusieurs types de carrefours, et des « carrefours qui n'en sont pas » (une **rue principale** traversée par d'autres, donc **asymétrique**). Même un périphérique en vrai cercle ne s'écrit pas simplement, ni un générateur de ronds-points. **« Les problèmes, ce n'est pas du cas limite, c'est LE cas. »** Il faut que ce soit **béton**.
- **Ce que le graphe apporte quand même** : la **topologie** (qui se connecte à quoi, quelle rue est prioritaire, quelle bretelle mène où). La **géométrie** reste la partie difficile.
- **Pour que ce soit béton** (Claude) :
  - ~~s'appuyer sur ASAM OpenDRIVE~~ → **non** (auteur, 2026-10-05) : il le pratique au travail, et **pas d'autoroutes** ici. Les routes du jeu sont des **chemins médiévaux**, des **villes**, des **treks**, des **chemins de montagne**. L'auteur a déjà un catalogue de cas (au travail) : un catalogue est évident ;
  - **une hiérarchie des routes** : la rue principale reste continue, les rues secondaires viennent s'y raccorder (d'où l'asymétrie) ;
  - **un catalogue de cas** (T, croix, rond-point, rue principale traversée, bretelle, raccord de largeurs différentes) avec une **map de test par cas**, et des **tests automatiques** qui vérifient les propriétés : continuité G1 / G2 mesurée, pas de chevauchement, voies bien raccordées. Comme pour Watershed et Rope.
- **Réflexion de l'auteur** : un rond-point, « chez moi, ce n'est pas une intersection… mais ça pourrait ». C'est **aussi un cul-de-sac** (une boucle de retournement). Une réflexion utile aussi pour son travail.
- **La simplification est un atout** : un jeu n'a pas besoin de fidélité, mais de **lisibilité**, ce qui est le message de Lynch. Un jumeau numérique doit être exact ; un lieu de jeu doit être **reconnaissable et jouable**.
- **Pas un open world, mais un monde en graphe**, comme les actes de *Diablo 2* : des zones reliées par des chemins (et des points de passage). Le graphe des zones est le plan du métro du monde ; seule la zone où se trouve le joueur existe en détail (principe n° 3).
- **Commencer petit** : une maison dans un quartier, puis le quartier, la ville en dernier. Les carrefours viennent avec le quartier.

## Questions ouvertes

- Premier usage : routes, murs, rivières, câbles ?
- Éditeur seulement, ou modifiable en jeu aussi ?
- ~~Reprendre `CitySplineTools` ?~~ → non : à part, voire à virer (auteur).
