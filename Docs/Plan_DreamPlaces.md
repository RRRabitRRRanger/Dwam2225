# Plan — Lieux rêvés : du hasard qui reste reconnaissable

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Graph]], [[Plan_PatchTerrain]], [[Plan_WorldState]], [[Plan_ProceduralTrees]]

> Statut : 💭 **idée** de l'auteur (2026-10-05), pour la génération aléatoire de DWAM. Rien n'est décidé.

## L'idée (auteur)

L'auteur fait des rêves avec des **lieux récurrents**, existants ou non :
- **son village d'enfance** (un lieu-dit), avec des variations mais **toujours très reconnaissable** : le tracé n'est pas exactement le même, une « feuille » est remplacée (une mine à la place d'une prairie, une autre forme, un lac…) ;
- **un « Paris rêvé »** qui ne ressemble pas à Paris, mais qu'il interprète comme Paris : il y a des **rives**, le grand cercle du **périphérique**, et un « Louvre » qui n'a rien à voir avec le Louvre, sauf que c'est un lieu culturel.

Il veut du hasard dans DWAM qui fonctionne comme ça : **varier sans perdre l'identité du lieu**.

## Lecture (Claude)

### Pourquoi un lieu rêvé reste reconnaissable

L'urbaniste Kevin Lynch (*The Image of the City*, 1960) a montré qu'on se souvient d'une ville par **cinq éléments**, pas par sa géométrie :
- les **voies** (les chemins qu'on emprunte) ;
- les **limites** (une rive, le périphérique : ce qui sépare) ;
- les **quartiers** (des zones avec un caractère) ;
- les **nœuds** (les carrefours, les places) ;
- les **repères** (le « Louvre » : un lieu qu'on identifie à son rôle).

Le Paris rêvé garde exactement ça : des rives (limites), un cercle (une voie fermée sur elle-même), un repère culturel. Le reste peut changer.

### Le graphe porte l'identité, la géométrie porte la variation

C'est la décision prise pour `GraphKit` ([[Plan_Graph]]) : **le socle n'a pas de position**. Ici, ça devient un principe de génération :
- **Le graphe d'un lieu = son identité** : quels endroits existent, leur **rôle** (tags : repère culturel, prairie, rive…), comment ils sont reliés, à quelle distance à peu près.
- **Chaque génération = une nouvelle mise en espace** du même graphe : le tracé des chemins varie, les formes varient.
- **Ce qui ne doit pas bouger**, c'est le **cœur** du graphe : ses **cycles** (le périphérique est une boucle), ses **passages obligés** (un pont sur la rive), ses repères.
- **Ce qui peut changer**, ce sont les **feuilles** (les endroits en bout de chemin) : une prairie remplacée par une mine ou un lac, **un autre endroit du même genre**, exactement comme dans le rêve.

### Pour creuser (l'auteur veut « creuser jusqu'à toucher le fond »)

- **Kevin Lynch**, *The Image of the City* (1960) : les cinq éléments de l'image mentale d'une ville. Le point de départ.
- **Joris Dormans**, *Adventures in Level Design: Generating Missions and Spaces for Action Adventure Games* (atelier PCG, 2010) : un graphe des missions, puis un graphe de l'espace, par grammaires de graphes.
- **Joris Dormans**, *Engineering Emergence* (thèse, 2012), et **Adams & Dormans**, *Game Mechanics: Advanced Game Design* (2012) : les grammaires de graphes, et **Machinations**, un langage de stocks et de flux pour les économies de jeu (voir aussi [[Plan_VitalPotential]]).
- **Unexplored** (Ludomotion, 2017) : la génération **cyclique**, des niveaux construits autour de boucles. Bien expliquée sur le blog de **Boris the Brave** (boristhebrave.com), qui écrit beaucoup sur la génération par graphes.
- **Shaker, Togelius & Nelson**, *Procedural Content Generation in Games* (2016), livre gratuit en ligne : le chapitre sur les grammaires et les graphes.
- Pour aller plus loin : **Hillier & Hanson**, *The Social Logic of Space* (1984), la « syntaxe spatiale » (quels lieux d'un réseau sont centraux) ; **Christopher Alexander**, *A Pattern Language* (1977), des motifs de lieux réutilisables.

## Plusieurs échelles d'un même lieu (auteur, 2026-10-05)

L'auteur, qui « ne sait pas le formuler », rapproche :
- **le plan du métro de Paris**, archi simplifié ;
- **Paris dans Google Maps**, chaque mètre carré zoomé mis bout à bout ;
- **Skyrim dans le lore**, raconté depuis Morrowind : une immense province ;
- **Skyrim le jeu**, avec une capitale plus petite que son village d'enfance.

### Une formulation (Claude)

**Un même lieu existe à plusieurs échelles de représentation**, et toutes sont vraies :
- **le schéma** (le plan du métro) : seulement la topologie, qui est relié à quoi. Le plan du métro de Londres de Harry Beck (1931) a inventé ce principe : garder les connexions, oublier la géographie ;
- **le détail** (Google Maps zoomé) : toute la géométrie ;
- **le récit** (Skyrim raconté) : une échelle de sens, où un voyage « dure des semaines » ;
- **le jeu** (Skyrim joué) : une échelle **compressée** pour rester jouable.

C'est exactement le graphe sans position : **le graphe est le plan du métro**, la mise en espace est la carte détaillée, et les **distances du graphe** peuvent être des distances **racontées**, que le jeu compresse.

### Le lien avec *A-Life*

Dans *S.T.A.L.K.E.R.* (2007), le système *A-Life* fait vivre les personnages **loin du joueur** de façon abstraite (ils se déplacent sur un graphe de la zone), et **près de lui** en détail. En une phrase :

> **Loin du joueur, le monde vit sur le plan du métro ; près de lui, sur la carte détaillée.**

Les factions d'Eiréné Nexus, l'économie du potentiel vital, les rumeurs vivent sur le graphe ; la géométrie n'existe que là où le joueur regarde. C'est aussi le principe « bang for buck » : on ne détaille que ce que le joueur peut sentir.

### Les bâtiments aussi

Avec le kit et des règles de mise en page ([[Plan_KitbashTool]]), le même graphe donne un bâtiment à n'importe quelle échelle : Fort-Dragon compact, à la *Skyrim*, ou grandiose à la *Witcher 3*. Les portes et les marches restent à taille humaine, les salles s'étirent.

### Pour creuser

- **Harry Beck**, le plan du métro de Londres (1931) : la carte schématique.
- La **généralisation cartographique** : comment une carte simplifie quand on dézoome.
- Les **graphes hiérarchiques** : un nœud (Paris) contient un sous-graphe (ses quartiers), qui en contient d'autres (ses rues).

## Les arbres, même principe

L'auteur l'applique aussi aux arbres (le feuillage) : **un graphe → plusieurs arbres crédibles**, même construits en kit. C'est plus de variation que l'habituel « arbre en kit, tourné au hasard ». Voir [[Plan_ProceduralTrees]].

## Questions ouvertes

- Les lieux rêvés sont-ils **des lieux du monde de DWAM** qui reviennent (un village qu'on retrouve d'un acte à l'autre, transformé), ou **des séquences de rêve** ?
- Le joueur doit-il **reconnaître** le lieu (c'est le but), ou s'en rendre compte peu à peu ?
- ~~Qui écrit le graphe ?~~ → **à la main** : il faut de l'**authoring**, c'est sûr (auteur, 2026-10-05). Un essai dans Obsidian a été « horrible ». Générer un graphe à partir d'une source, ou du hasard : « cool en principe ».
- **« Plus tard »** (précisé par l'auteur : Claude l'avait mal compris) : une fois les endroits faits, **les faire vivre**, façon *A-Life* en gros, en plus léger. Voir la section « Plusieurs échelles » ci-dessus.
- **L'outil d'écriture des graphes** : voir [[Plan_Graph]] (éditeur de graphes dans Unreal).
