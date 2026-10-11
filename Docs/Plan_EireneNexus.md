# Plan — Eiréné Nexus : factions organiques et théorie des jeux

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Loot]], [[Plan_WorldState]], [[Plan_KlattVoice]], [[Guide_StateTree]], [[Plan_3C]]

> Statut : 💭 **idée**, en discussion avec l'auteur (2026-10-04). Rien n'est décidé. Les notes d'origine de l'auteur sont perdues ; cette page reprend ce qu'il en a redit.

## Intention (auteur, 2026-10-04)

- Un **système de factions** qui tourne sur des **principes de théorie des jeux**, et dont il sort **des histoires que personne n'a écrites**. C'est la seule chose gardée de l'esprit du Nemesis system.
- **Le nom est volontairement l'opposé** : Némésis, la vengeance ; Eiréné, la paix. L'auteur ne veut ni poser ni avoir de problème. Les systèmes de ce genre existent depuis longtemps : *Dwarf Fortress* (histoires générées, relations, légendes) en est l'exemple.
- **On ne reprend pas** les mécaniques propres au Nemesis system de Warner Bros. (protégé par un brevet américain).

## Le modèle de l'auteur

- **Des factions par acte**, comme dans *Diablo 2* (ou *Grim Dawn*) :
  - certaines sont **« en dur »**, pensées comme un cadre, pas censées bouger ;
  - dans un autre acte, elles peuvent être **plus fluides**.
- **Les factions se forment à partir de tags** : des ensembles de **valeurs et d'objectifs**, des **goûts et des dégoûts**.
- **Par proximité** de ces groupes, des factions et des **sous-factions** apparaissent. C'est plus **organique** qu'un organigramme écrit à la main.
- **Entrer dans une faction, en sortir** : voulu. **Les luttes internes**, le côté organique : voulus.
- **Les entités** qu'on croise dans les groupes : certaines ont des **fonctions avancées**, dont une **mémoire**, d'où des **rumeurs**.
- **Ça fonctionne avec un autre système**, qui doit **aussi** fonctionner seul : le **potentiel vital** ([[Plan_VitalPotential]]), la vie comme monnaie cachée.

## Le principe « bang for buck »

Les PNJ d'*Oblivion* suivent un emploi du temps : techniquement beau, mais beaucoup de travail pour un joueur qui ne le voit pas. **On ne simule que ce que le joueur peut sentir.**

L'exemple de l'auteur : dans *Elden Ring*, l'un des premiers boss vers lesquels le jeu nous pousse ordonne au joueur de s'agenouiller. **Pouvoir le faire**, avec des conséquences, serait « cool, et RPG ». Dans le jeu, ça ne sert à rien.

## Proposition (Claude) : des valeurs dans un espace, des factions qui en émergent

- **Chaque entité a un profil de valeurs** : des tags pondérés (ex. Or +0,9, Honneur +0,5, Magie −0,7). Ce sont ses goûts et ses dégoûts.
- **La proximité** entre deux entités = la ressemblance de leurs profils **et** la proximité géographique (décision de l'auteur : « les deux » ; sinon, sur une grande carte, des groupes qui ne se sont jamais vus formeraient soudain le même groupe). Les liens sociaux peuvent s'y ajouter.
- **Les factions sont des regroupements** dans cet espace. Un regroupement **hiérarchique** donne naturellement des **sous-factions** : les branches d'un même arbre.
- **Les factions « en dur »** sont des **centres fixes**, posés par le design : elles attirent des membres mais ne bougent pas.
- **Entrer, sortir** : quand le profil d'une entité s'éloigne du centre de sa faction (événements, influence du joueur), elle se rapproche d'une autre, ou fait sécession. Un **seuil avec une marge** (hystérésis) évite de changer de camp à chaque contrariété.
- **Luttes internes** : quand une faction s'étire dans l'espace des valeurs, deux sous-groupes divergent. C'est là que naissent les schismes.
- **Les gains de la théorie des jeux viennent des valeurs** : ce qu'une faction gagne ou perd dans une situation se calcule à partir de ce qu'elle aime et déteste.

### Les gestes du joueur sont des signaux

C'est l'exemple de l'agenouillement. Un geste (le menu d'emotes de [[Plan_3C]]) est lu par le système comme un **signal** :
- s'agenouiller devant un chef qui valorise la soumission : un signal de soumission, avec ses suites (rejoindre, payer un tribut, être humilié, gagner son respect, ou tomber dans un piège) ;
- refuser : un défi, lui aussi lu.

La théorie des jeux distingue le **signal gratuit** (des mots) du **signal coûteux** (abandonner son arme, payer). Seul le second rend une promesse crédible.

## Les principes de théorie des jeux utiles

| Principe | Entre factions |
|---|---|
| **Dilemme du prisonnier répété** (Axelrod) | La coopération émerge quand on s'attend à se revoir longtemps. Les stratégies deviennent des personnalités : donnant-donnant, pardon occasionnel, rancune éternelle |
| **Faucon / colombe** | Céder, menacer ou se battre pour une ressource, selon ce qu'on risque |
| **Chasse au cerf** | Une alliance rapporte plus, mais seulement si l'autre tient parole |
| **Dilemme de sécurité** | S'armer inquiète le voisin, qui s'arme : la course aux armements sans que personne la veuille |
| **Négociation, option de repli** | Un accord dépend de ce que chacun obtient sans accord |
| **Coalitions** | Contre le plus fort : s'allier contre lui, ou le rejoindre |
| **Signaux coûteux, engagement crédible** | Une promesse ne vaut que si la rompre coûte cher |
| **Biens communs** | Une ressource partagée (un bassin, une forêt) s'épuise si chacun tire pour soi |
| **Information incomplète** | Chaque faction a des croyances, parfois fausses : rumeurs, espions, faux documents |
| **Dynamique évolutive** | Les stratégies gagnantes se répandent : une faction change de culture |

## Liens avec les autres systèmes

- **Loot** ([[Plan_Loot]]) : les marques peuvent être les ateliers des factions, et leur histoire **produire les périodes** (guerre, paix, rachat). Les contrefaçons deviennent des armes politiques.
- **État du monde** ([[Plan_WorldState]]) : l'histoire des factions est un état posé sur la graine du monde ; les souvenirs s'estompent.
- **Voix** ([[Plan_KlattVoice]]) : les rumeurs se transmettent par la parole.
- **3C** ([[Plan_3C]]) : les emotes sont des signaux sociaux.
- **IA** ([[Guide_StateTree]]) : la stratégie d'une faction devient des ordres pour ses membres.
- **Eau, feu** : ressources et catastrophes nourrissent les conflits.

## Questions ouvertes

- Que porte **Eiréné** : la paix comme but du joueur, ou comme équilibre fragile du monde ?
- **Le rôle du joueur** : membre d'une faction, indépendant, médiateur, agitateur ?
- **Ce que le joueur voit** : une carte des relations, ou seulement des indices (rumeurs, patrouilles, prix, objets) ?
