# Plan — Potentiel vital (la vie comme monnaie)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_EireneNexus]], [[Plan_Loot]], [[Plan_Gauges]], [[Plan_WorldState]]

> Statut : 💭 **idée**, en discussion avec l'auteur (2026-10-04). Un prototype naïf existe. Rien n'est codé.

## Intention (auteur, 2026-10-04)

- **Constat** : l'auteur n'aime pas les jeux qui veulent de la tension mais où la mort ramène simplement en arrière, sans intérêt (*Mass Effect* mis à part, « parfait »). Il n'aime pas non plus la **rupture de continuité** des rogue-likes.
- **Une monnaie, la vie**, qu'on **ne nomme pas** au joueur.
- **Tuer un ennemi = absorber son potentiel vital** :
  - **une partie va dans l'équipement**, pour le modifier ;
  - **une partie va dans le personnage**, comme de l'XP ou une vie en plus (*1-up*).
- C'est **l'autre système** qui doit fonctionner avec [[Plan_EireneNexus]], **et seul**.

## Les règles posées par l'auteur

1. **Un équilibre par acte, conservé à l'échelle de l'acte.** Le phare : *Diablo 2*. Chaque acte a son économie fermée.
2. **Somme nulle.** Le système tourne **légèrement, en fond**, à la manière d'un **jeu incrémental**, mais rien ne se crée.
3. **Les factions dépensent leurs points comme dans un RTS.** Exemple : Andariel aurait beaucoup de points à répartir ; « la Duchesse » serait une autre faction (`À COMPLÉTER` : qui est-elle ?).
4. **1 vie = 1 vie**, mais **on ne récupère pas tout** en tuant : il y a une perte, et **l'équipement a tendance à se servir avant le joueur**.
5. **La nature a horreur du vide** : la vie libérée ne reste pas vacante.
6. **Si un ennemi tue le joueur, il gagne une vie.** Pas de suivi pour un ennemi lambda ; un **élite** sera probablement plus fort, et suivi. Mais c'est surtout **une ressource pour la faction**.
7. **Un acte peut ponctionner l'énergie d'un autre acte.**
8. **L'équilibre crée les ambiances** : ressources rares ou abondantes, le monde ne se vit pas pareil.
9. **Revenir à la vie** : des **potions de résurrection** et des **autels**. Le joueur ne réapparaît pas au hasard. Plusieurs natures, pour varier et surprendre, mais toujours **une méthode simple et familière**, pour ne pas perdre le joueur.
10. **La magie y puise aussi**, comme si l'on jouait avec des **boules de neige** : une boule que tu me lances est une munition que tu me donnes.
11. **La ponction entre actes est décidée par l'histoire** (le développeur), avec des **outils** pour la régler. L'idéal : que le joueur **pense pouvoir casser le jeu**.
12. **Exemple d'émergence** : la Comtesse (*the Countess*) de *Diablo 2*, au fond de sa tour. Le joueur enchaîne les missions émergentes contre Andariel ; la Comtesse en profite ; à la fin, le boss n'est plus Andariel mais la Comtesse. « Dans l'idée, comme exemple. »
13. **On ne peut pas mourir** (précisé le 2026-10-04). À la place, des **événements de mort imminente**, variés :
    - **sauvé** : le pouvoir de l'amitié ; mis KO puis sauvé par une troupe de gardes ;
    - **capturé** : réduit en esclavage, dans une mine, dans une arène de gladiateurs ;
    - **ramené** : par un nécromancien, par un dieu ;
    - **interrompu juste avant le coup fatal** : une éclipse qui fait paniquer, une chute de météore, un autre groupe de bandits qui attaque ceux qui t'avaient attaqué.

## Le prototype (lu par les noms des assets)

Selon l'auteur : « une tentative naïve de mettre en place le système, un aperçu ».

- `Content/RPG/Systems/Gauge/BP_EcoSystemVP_01b` : « Potentiel vital total » (`Global VP`) et `Factions Populace`, une table par population (`Player`, `Boss`, `Rogue`, `Neutrals`, `Nice`, `Bunnies`, `Beddies`, `vegetation`) en entiers 64 bits.
- `Content/RPG/Characters/p1/WBP_EcoSystemVP_01a` : l'affichage.
- `Content/RPG/Systems/BPC_BigNumbers` + `S_BigNumbers` : de très grands nombres (mantisse et exposant).

## Lecture (Claude)

- **Un modèle de stocks et de flux.** Chaque acte est un ensemble de **réservoirs** : chaque faction, la nature, le joueur, son équipement. Des **flux** vont de l'un à l'autre : tuer, mourir, dépenser pour faire apparaître des unités, lancer un sort, la ponction d'un autre acte. Le total d'un acte ne change que par ces ponctions entre actes. C'est la même discipline que Watershed, où l'eau est conservée et vérifiée par des tests.
- **« La perte » ne disparaît pas** : pour rester à somme nulle, elle doit aller **quelque part**. La nature, qui « a horreur du vide », est le candidat naturel : la vie perdue au combat retourne à la végétation et à la faune, qui la rendent plus tard. Ça donne aussi une règle simple pour le vide : la vie libérée est réclamée par le réservoir voisin le plus « affamé », ou par le plus fort. En théorie des jeux, c'est un bien commun.
- **Le côté incrémental** : des taux de flux (repousse, recrutement, ponction), évalués à intervalle régulier, pas à chaque frame. Peu coûteux.
- **Grands nombres** : à somme nulle, le total d'un acte est **borné**. Des entiers 64 bits (jusqu'à environ 9 × 10¹⁸) suffisent largement ; la mantisse et l'exposant ne sont utiles que si les valeurs explosent, ce que la somme nulle empêche.
- **Les ambiances par l'équilibre** : on ne règle pas « l'ambiance », mais les réservoirs de départ et les taux. La rareté ou l'abondance en découle. Les factions **en dur** d'[[Plan_EireneNexus]] peuvent être des réservoirs dont le design fixe l'état.
- **La magie en boules de neige** : un sort transporte du potentiel. S'il touche, il en donne à la cible, ou à la nature. S'il est **paré ou attrapé**, il en devient une munition pour le défenseur. Ça rejoint la parade au poignet de [[Plan_3C]].
- **La mort imminente reste à somme nulle** : quelqu'un **paie** la vie qui sauve le joueur.
  - Les gardes perdent des hommes pour te sauver, et ta **dette** envers leur faction est réelle. C'est la réciprocité du dilemme répété ([[Plan_EireneNexus]]) : rendre service, ou trahir.
  - Le nécromancien ou le dieu prend **un prix**.
  - L'esclavage, c'est **rembourser** sa vie par le travail, en jouant une séquence (mine, arène) sans rupture de continuité.
  - Les potions et les autels sont des **réserves de vie** : une forme de sauvetage parmi les autres (confirmé par l'auteur, 2026-10-04).
- **Le prix d'un sauvetage** (auteur, 2026-10-04) : probablement **de l'argent**, **un peu de loot**, **un one-up** : le joueur **perd l'un des trois, ou les trois** (confirmé, 2026-10-04). Ce qu'il perd va à celui qui l'a sauvé ou capturé : on peut recroiser plus tard un esclavagiste qui porte son ancienne épée.
- **Choisir l'événement** : selon qui est à proximité et ses relations avec le joueur (une faction amie près de là → sauvetage ; des esclavagistes → capture), le potentiel disponible dans l'acte, et l'endroit. C'est une sélection par utilité, comme l'IA dans le State Tree, et c'est ce qui rend l'événement surprenant sans être arbitraire.
- **Les outils de la ponction** : un tableau de bord des réservoirs et des flux par acte, une frise du temps, et une **simulation « et si »** (accélérer l'économie d'un acte pour voir où elle va). C'est aussi ce qui permet de rendre le jeu « cassable en apparence » sans qu'il le soit vraiment.
- **L'exemple de la Comtesse** est la somme nulle en action : affaiblir une faction nourrit sa voisine. Le boss final d'un acte peut ainsi changer selon le jeu du joueur.
- **Avec le loot** : l'équipement « se sert avant le joueur ». Une arme qui absorbe change de profil, et son histoire s'écrit (la lame qui a bu la vie de tel chef). Ça compte dans sa valeur de collection ([[Plan_Loot]]).
- **Avec Eiréné Nexus** : le potentiel d'une faction est sa puissance. C'est ce qu'elle dispute aux autres et ce qu'elle dépense.

## Référence : Machinations

Le modèle de stocks et de flux du potentiel vital a un outil dédié : **Machinations** (Joris Dormans ; *Game Mechanics: Advanced Game Design*, Adams & Dormans, 2012), un langage graphique pour concevoir et simuler les économies de jeu (réservoirs, flux, conversions). Utile pour prototyper l'équilibre d'un acte avant de coder.

## Questions ouvertes

- **Où va la perte** quand on tue : à la nature, comme proposé ?
- ~~Si le joueur n'a plus de vie en réserve~~ → **on ne meurt pas** : événements de mort imminente (règle 13).
- ~~La ponction entre actes~~ → **l'histoire**, avec des outils (règle 11).
- ~~La Duchesse~~ → **la Comtesse** de *Diablo 2* (règle 12).
- **Où va la perte** quand on tue : à la nature, comme proposé ?
- **Caché mais ressenti** : comment le joueur sent-il cette monnaie sans qu'on la lui nomme ?
- **Le partage** équipement / personnage : fixe, selon l'arme, selon l'archétype ?
- ~~Potions et autels~~ → **oui**, une forme de sauvetage parmi les autres.
- ~~Que garde le joueur~~ → il **perd** de l'argent, un peu de loot ou un one-up (l'un, ou les trois).
