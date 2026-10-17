# Plan — Visages des PNJ

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Eyes]], [[Plan_NPC]], [[Plan_NormalTransfer]], [[Plan_KlattVoice]], [[Catalogue_Features]]

> Statut : 💭 **idée**, « une vague idée, il faut la mettre au point » (auteur, 2026-10-17). Rien n'est décidé, rien n'est codé.

## Intention (auteur, 2026-10-17)

- **Un visage expressif sur un mesh presque rigide.** Piste partagée avec Claude : l'expression vient en grande partie du shader. Les paupières de [[Plan_Eyes]] (plugin `SDFEyes`) en sont la première pièce.
- **Le mesh n'a pas à être totalement rigide** : à l'ancienne, on peut **mettre à l'échelle une partie du visage**, par exemple pour faire bouger la mâchoire.
- **Référence : l'animation en stop motion** (la manière dont certains films procèdent), avec un atout : l'outil maison de transfert de normales ([[Plan_NormalTransfer]]), « inclus dans les outils, facilement accessible avec le path en custom maintenant ».
- **Sourcils et bouche aussi**, la bouche pilotée par la synthèse vocale ([[Plan_KlattVoice]]).
- **Sourcils** : l'auteur doute qu'ils aient besoin de SDF, mais **sourcils et yeux devront fonctionner main dans la main**.
- **Textures** : de vraies textures, tout au plus générées de façon procédurale.
- **Vues visées** : « pense Skyrim, pense Deus Ex, Mass Effect aussi ». Donc des **dialogues de près**, avec des gros plans sur les visages des PNJ.
- **Antécédent** : de vieux tests de l'auteur dans Blender (perdus) : un mesh réaliste d'un seul bloc, avec de légers creux pour les orbites, et une parallaxe « étrange, naïve », sans SDF.

## Précisions de l'auteur (2026-10-17, suite)

- **Penser l'ensemble** : `SDFEyes` ([[Plan_Eyes]]) ne doit pas rester une île, il doit fonctionner avec le visage. Ce plan est l'endroit où l'on pense le tout.
- **Des yeux n'importe où, pas forcément par paire**, éventuellement un troisième œil (détail dans [[Plan_Eyes]]). Le visage ne suppose donc pas « deux yeux symétriques ».
- **Un éditeur de personnage complet** sera nécessaire.
- **Une tête modulaire** : mailler une tête entière est complexe, d'où l'idée de la construire en modules. Ça rejoint les pièces rigides de [[Plan_NPC]], la référence stop motion et le transfert de normales pour cacher les coutures, et peut-être l'outil de composition par formes ([[Plan_KitbashTool]]).

## Lecture de Claude (à valider)

- **Le stop motion par remplacement** (de mémoire, à vérifier) : des studios comme Laika impriment des milliers de visages et les remplacent image par image ; le visage est souvent coupé en deux, haut et bas, au niveau des yeux et du nez. L'équivalent ici : **des pièces de visage interchangeables** (bas du visage, mâchoire, bouche) qui se remplacent ou se mettent à l'échelle. La **couture** entre les pièces est cachée par le transfert de normales. C'est exactement le problème que cet outil résout déjà entre les pièces rigides du corps.
- **Ce que le shader prend en charge** : les paupières et le regard (`SDFEyes`), les sourcils (dessinés sur la surface, sans SDF a priori), les rides qui apparaissent avec l'expression, peut-être les détails de la bouche (lèvres, intérieur).
- **Bouche et synthèse vocale** : un synthétiseur de Klatt manipule des grandeurs proches de l'articulation (l'ouverture de la mâchoire suit à peu près le premier formant, l'arrondi des lèvres abaisse les formants suivants). Piste : **piloter la bouche directement par les paramètres du synthétiseur**, sans analyser le son. À vérifier dans [[Plan_KlattVoice]].
- **Gros plans de dialogue** (Skyrim, Deus Ex, Mass Effect) : c'est le cas le plus cher pour `SDFEyes`. La version analytique (étape U3 de [[Plan_Eyes]]) devient nécessaire, et la qualité doit tenir de près.
- **Vue subjective** : le joueur ne voit presque jamais son propre visage ; l'effort porte sur les PNJ.

## Expressions en couches (auteur, 2026-10-18)

**Critique du FACS apportée par l'auteur** : c'est un système **anatomique, musculaire et atomique** (AU1 = haussement de l'intérieur du sourcil, AU12 = coins des lèvres tirés…). Pour un sourire triste, il faut doser six curseurs à la fois, et les combinaisons créent souvent des interpénétrations laides, qui demandent des « corrective blendshapes ».

**Principe retenu par l'auteur** : découper le visage non pas en muscles, mais en
1. **Visèmes** (phonèmes visuels) : environ 10 à 15 formes pures pour la parole (`A`, `O`, `E`, `MBP`, `FV`, `LNTD`…) ;
2. **Macro-expressions, émotions primaires** : des poses complètes ou d'un demi-visage (joie, colère, peur, dégoût, surprise, tristesse).

**Lecture de Claude** :
- **La critique est juste pour l'animation à la main.** Les correctives viennent de ce que des blendshapes s'additionnent en ligne droite. Ici, les paupières ne peuvent pas se croiser (leur ordre est garanti par construction), mais les champs de déformation du mesh (les lèvres surtout) le peuvent encore.
- **Les deux niveaux se complètent, ils ne s'excluent pas.** Les pros animent avec des poses (visèmes, émotions), construites au-dessus d'un petit jeu de pilotes de bas niveau. Proposition : **ce qu'on anime** = visèmes + émotions ; **ce qui déforme** = un petit nombre de pilotes propres à nos modules (bien moins que les ~44 unités du FACS). Chaque pose est un préréglage de ces pilotes ; les correctives éventuelles se règlent une fois, dans la pose.
- **Les poses de demi-visage tombent juste avec le découpage** : masque haut (yeux, sourcils) et masque bas (bouche, joues). Un **sourire triste** = le haut de « tristesse » + le bas de « joie », sans doser six curseurs.
- Les six émotions primaires viennent elles aussi d'Ekman, et les visèmes ont leurs tables classiques (la parole des dessins animés, les 15 visèmes d'Oculus). Ce sont aussi des standards, simplement d'un niveau plus utile pour animer.
- La parole et l'émotion se superposent : les **visèmes pilotent la bouche** (depuis [[Plan_KlattVoice]]), l'**émotion pilote surtout le haut** et teinte le bas.

## Questions ouvertes

- Les pièces du visage : quel découpage (haut / bas, mâchoire, bouche) ? Remplacement de pièces, mise à l'échelle, ou les deux ? `À COMPLÉTER`
- Les sourcils : forme dessinée, brins, ou texture déformée ? Avec quels pilotes communs avec les paupières (plissé, surprise, colère) ? `À COMPLÉTER`
- La bouche : quelle part en géométrie (mâchoire), quelle part en shader (lèvres) ? `À COMPLÉTER`
- La variété des visages : faits à la main, générés, ou générés puis retouchés ? `À COMPLÉTER`
- ~~L'éditeur de personnage : pour qui ?~~ → **pour l'auteur ET pour le joueur** (auteur, 2026-10-17). Conséquence : il doit tourner **en jeu**, donc sans outils réservés à l'éditeur Unreal.
- ~~Les modules de la tête~~ → proposition de l'auteur (2026-10-17) : **haut de la tête, masque, mâchoire, oreilles et nez**, avec peut-être quelques détails en plus. Avis de Claude et méthode des pros ci-dessous.

## Découpage de la tête : avis de Claude (2026-10-17, à valider)

**Ce que font les pros** (de mémoire, à vérifier avant de s'en servir) :
- **Skyrim, Mass Effect, Deus Ex, MetaHuman** : **un seul mesh de visage**, avec la **même topologie pour tout le monde**. L'éditeur de personnage déforme ce mesh par des curseurs (morph targets, parfois des os du visage mis à l'échelle). Des pièces s'ajoutent par-dessus : yeux, dents, langue, cheveux, barbe, sourcils, cicatrices (les « head parts » de Skyrim). MetaHuman Creator mélange des visages scannés **par régions** (front, yeux, nez, bouche, mâchoire), mais toujours sur une seule topologie.
- **Pourquoi** : une seule topologie permet à **toutes les animations faciales de marcher sur tous les visages** (un sourire bouge la bouche, les joues et les paupières inférieures en même temps, à travers les régions) ; pas de couture qui s'ouvre quand le visage se déforme ; une texture continue.
- **Le découpage en pièces** existe surtout là où le visage ne se déforme pas : le stop motion (haut et bas du visage séparés au niveau des yeux, pour remplacer la bouche sans toucher aux yeux) et les jeux qui échangent des accessoires.

**Pour DWAM, ton découpage est cohérent**, parce que les raisons des pros ne s'appliquent pas : pas de skinned mesh, expression surtout portée par le shader (paupières, sourcils), coutures cachées par le transfert de normales. Ajustements proposés :
1. **Placer les coutures là où elles se cachent** : la lisière des cheveux (haut / masque), sous la mâchoire, la jonction des oreilles. Éviter les joues et le front, où l'œil lit les reflets.
2. **La mâchoire tourne autour d'une charnière** (sous l'oreille), en plus de l'échelle : c'est le mouvement réel de l'ouverture.
3. **Le masque en deux, haut et bas** (comme le stop motion), si la bouche et les joues doivent changer de forme sans toucher aux yeux : un masque haut (front, arcades, orbites, pommettes, arête du nez) qui porte les yeux et les sourcils ; un masque bas (lèvre supérieure, sillons, bas des joues) qui accompagne la bouche.
4. **Nez et oreilles comme pièces posées et interchangeables**, à la manière des « head parts » : c'est là que la variété se voit le plus.
5. **Les formes dans l'éditeur** : sans skinned mesh, Unreal n'a pas de morph targets sur les static meshes. Deux pistes : déformer dans le **vertex shader** (World Position Offset piloté par les curseurs), ou **générer le mesh une fois** à la création du personnage (Dynamic Mesh à l'exécution). ~~À trancher~~ → **vertex shader** (décision de l'auteur, 2026-10-18).

**Décision de l'auteur (2026-10-18)** : les cinq ajustements sont **acceptés**, et les formes se déforment **dans le vertex shader**.

## Les expressions marchent sur tous les visages, proportionnellement (2026-10-18)

Question de l'auteur : « c'est pareil pour nous, non ? Juste proportionnellement ? » Réponse de Claude : **oui, et c'est même un point fort, à une condition.**

- **Une expression est un jeu de paramètres, pas un déplacement de sommets.** « Sourire à 70 % » donne les mêmes nombres pour tout le monde, appliqués à la forme propre de chaque visage. Elle s'adapte donc proportionnellement, sans retargeting. Les pros, eux, doivent garder une topologie commune pour obtenir ça.
- **La condition : un sourire traverse les modules.** Il remonte les coins de la bouche (masque bas), soulève les joues (masque haut, pommettes) et remonte les paupières inférieures (`SDFEyes`). Il faut donc :
  1. **définir chaque expression comme un ensemble coordonné** de paramètres sur tous les modules et sur `SDFEyes` ;
  2. **calculer les déformations dans un champ commun à la tête**, évalué de la même façon par chaque pièce, dans les coordonnées de la tête. Deux pièces voisines bougent alors exactement pareil à leur couture, qui ne s'ouvre pas ;
  3. **exprimer ces champs par rapport à des repères du visage** (ancres des yeux, coins de la bouche, charnière de la mâchoire), pour qu'ils suivent les proportions de chaque visage.
- **Vocabulaire proposé : le FACS** (*Facial Action Coding System*, Ekman et Friesen), la référence pour décrire les expressions en « unités d'action ». Exemple de mémoire : sourire franc = AU6 (soulève les joues) + AU12 (tire les coins des lèvres). Chaque unité d'action devient un pilote qui touche plusieurs modules à la fois.
- ~~**L'auteur valide le FACS**~~ → nuancé par l'auteur juste après, voir « Expressions en couches » ci-dessous.
- Note d'origine : l'auteur valide le FACS (2026-10-18, « je pensais aller là-dessus »). Contexte (Claude, de mémoire) : publié en 1978, révisé en 2002 ; vieux, mais toujours la référence. Les 52 blendshapes d'ARKit, qu'utilise Live Link Face sur iPhone, en sont inspirés : piste pour **piloter un jour les visages par capture avec un téléphone**. Limites : le FACS décrit des mouvements visibles, pas des émotions, et ne couvre ni la parole (les visèmes viennent à part, depuis [[Plan_KlattVoice]]) ni la langue.
