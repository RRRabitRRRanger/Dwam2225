# Plan — Dialogue : répliques, gestes, combats qui parlent

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_EireneNexus]], [[Plan_3C]], [[Plan_VitalPotential]], [[Plan_KlattVoice]], [[Plan_Face]], [[Plan_VerticalSlice]], [[Feuille_de_route]] (J5), [[References_Precedents]] (section 8), [[Confrontation_Gemini_Narration]]

> Statut : 📐 **intention et scène validées** (2026-10-25). La conception technique (faits, règles, format d'écriture, intégration Unreal) reste à écrire ; rien n'est codé.

## Intention (auteur, 2026-10-25)

- **Le joueur choisit ses répliques.** Du RPG, à la *Mass Effect*, avec moins de contenu : l'auteur pourrait payer des gens pour une partie du travail de pro, à l'échelle d'une tranche.
- **Deux sources** : le scénario posé (écrit) **et** des choses qui émergent (Eiréné Nexus, potentiel vital).
- **Les combats aussi sont du dialogue.** Combattre un ancien allié qui a changé de camp, en esquivant, en parant plutôt qu'en frappant : c'est du jeu de rôle, et le jeu doit le prendre en compte, **pas bêtement**.
- **Pas d'alternance de modes** (« moment RPG, moment action, marche, traversal, action »). Le jeu de rôle passe par tout ce que fait le joueur, **sans être présenté comme ça**. Références de l'auteur : *Dishonored*, *Deus Ex*.

## Lecture (Claude)

### Le problème a un nom

Clint Hocking (directeur créatif de *Far Cry 2*) a appelé **dissonance ludonarrative** (2007, à propos de *BioShock*) l'écart entre ce que l'histoire dit et ce que le jeu fait faire. Ce que veut l'auteur, c'est l'inverse : les gestes de jeu **sont** le récit.

### Précédents proposés par Claude, et l'avis de l'auteur (2026-10-25)

L'auteur confirme que les descriptions sont justes, mais **ce n'est pas ce qu'il veut reproduire** :

| Jeu | Ce que Claude en tirait | Avis de l'auteur |
|---|---|---|
| *Deus Ex*, *Dishonored* | Le monde réagit à **si** on tue | Pas ça : l'auteur met en avant **comment** on tue ou comment on épargne (voir ci-dessous) |
| *Mass Effect 2* (interruptions) | L'action entre dans le dialogue | « Sympa », pas prévu |
| *Metal Gear Solid 3* (The Boss) | Un combat qui raconte une relation | Un jeu couloir : scripté, les choses ne peuvent pas se passer autrement |
| *Undertale* | Combats résolus en parlant, en épargnant | Un bijou, mais pas une proposition qui le touche en tant que joueur |
| *Kingdom Come: Deliverance* | Redditions | L'auteur le connaît mal ; trop lourd, « plus simu que jeu » |
| *Alpha Protocol* | Répliques sous pression du temps | — |

### Ce que l'auteur met en avant : la manière de se battre (2026-10-25)

Dit « de manière simpliste » par l'auteur :
- **Foncer dans le tas**, prendre l'aggro et se défouler : c'est bien, c'est un jeu, le côté action **doit** marcher.
- Mais un joueur **surtout sur la défensive**, qui ne rend que la **réponse équivalente**, **comme s'il appliquait la légitime défense** : ça doit compter aussi.
- **L'anti-exemple : *Skyrim*.** On met un ennemi au sol, il crie qu'il se rend… puis se relève et recommence à se battre. Si on a « cru » cette présentation (pas une promesse, mais presque), qu'on se retourne et vaque à ses occupations, on se fait attaquer dans le dos.

**Lecture (Claude).** Deux exigences en sortent :
1. **Le style de combat est lu**, pas seulement l'issue : qui a frappé le premier, la proportion de la riposte, parer sans riposter, s'arrêter quand l'autre est à terre. C'est une mesure dans la durée, pas un compteur de morts.
2. **Une reddition est un vrai état, et elle engage.** Celui qui se rend arrête vraiment. S'il trahit sa reddition, ce n'est pas un bug d'IA : c'est un **acte** choisi par un PNJ dont les valeurs le permettent (peu d'honneur), lisible, et qui aura des conséquences (rumeurs, réputation). C'est le **signal coûteux** d'[[Plan_EireneNexus]] : une reddition ne vaut que si la rompre coûte cher.

## Réponses de l'auteur (2026-10-25)

- **Le personnage du joueur est une page blanche**, comme dans *Skyrim* ou *Diablo*.
- **Les répliques : à la *KOTOR*** (une liste de répliques en texte complet), **avec la voix de [[Plan_KlattVoice]]**. Lecture : dans *KOTOR*, le héros est muet et beaucoup de PNJ parlent une langue étrangère sous-titrée ; une voix synthétique, stylisée, sous-titrée y trouve naturellement sa place. Le joueur reste-t-il muet ? `À COMPLÉTER`.

### Ce que ça change pour le système

1. **Un seul canal : tout acte du joueur est un signal.** Une réplique choisie, un emote, s'agenouiller, rengainer, parer sans riposter, désarmer au lieu de tuer, épargner un homme à terre : tout devient un **événement** lu par les PNJ selon leurs valeurs et leur histoire avec le joueur. C'est déjà le principe de [[Plan_EireneNexus]] (« les gestes du joueur sont des signaux », signal coûteux).
2. **Le système de Ruskin convient au combat.** Il a été fait pour les répliques **pendant** le combat de *Left 4 Dead 2*. L'ancien allié peut parler selon la façon dont on se bat (« Tu ne frappes pas… tu te souviens encore ? »), sans couper l'action.
3. **Le combat peut ouvrir un dialogue, et le dialogue un combat, sans changer de mode.** Ici, c'est presque naturel : **on ne meurt pas** (principe n° 6) et la vie est une monnaie ([[Plan_VitalPotential]]). Un combat finit souvent autrement que par une mort : reddition, fuite, agenouillement, capture. Le combat devient une **négociation**.
4. **La main contrôlée dit l'intention.** Avec l'arme comme filtre de la main ([[Plan_3C]], principe n° 10), la façon de tenir et d'utiliser l'arme est déjà un langage : arme basse, coups retenus, parades seules.
5. **Les répliques du joueur** : écrites pour le scénario, choisies par le joueur ; les règles décident lesquelles sont proposées (selon ce qui s'est passé, y compris au combat) et comment le PNJ y répond.

## La scène du chef (J5), vue par le dialogue (proposition de Claude, 2026-10-25)

**D'où elle vient.** [[Plan_VerticalSlice]] (2026-10-04) construit la première version jouable autour **d'une scène**, partie de l'exemple d'*Elden Ring* de l'auteur ; « validée comme approche », **le boss peut changer**. Elle doit prouver **le monde, le côté RPG et la défaite sans mort**. La [[Feuille_de_route]] en fait le jalon **J5**, le dernier avant P1 : « dialogue minimal + emote, mort imminente, potentiel vital, loot minimal, mémoire ». C'est un **prototype de la boucle** (formes grises), pas une finition.

**La scène d'origine, en six temps** (de [[Plan_VerticalSlice]]) : un chef **exige** que le joueur se soumette → **se soumettre** (rejoindre, tribut, respect, ou piège) → **refuser** (le combat commence) → **perdre** (mort imminente : capturé au camp, ou sauvé par des gardes rivaux avec une dette ; prix : argent, loot ou one-up) → **gagner** (potentiel du chef absorbé, l'arme change ; ses hommes se dispersent, se soumettent, ou un rival prend la place) → **revenir plus tard** (le monde se souvient).

**Ce que le dialogue y ajoute, avec l'intention du 2026-10-25 :**

1. **L'approche.** Le joueur arrive au camp des bandits. Les sentinelles réagissent à ce qu'elles voient (arme dégainée ou non, ce qu'on sait du joueur) : quelques répliques selon des règles.
2. **L'exigence.** Le chef parle (Klatt, sous-titré). Le joueur a une **liste de répliques à la *KOTOR*** : se soumettre, refuser, négocier (un tribut), provoquer. Certaines n'apparaissent que si un fait le permet (une dette envers les gardes, un objet).
3. **Le geste.** S'agenouiller est **aussi** une réponse, sans passer par la liste : l'emote est un événement comme une réplique. Dégainer pendant qu'il parle, aussi.
4. **Le combat qui parle.** Si ça se bat, le chef et ses hommes **commentent la manière** : le joueur qui pare sans riposter, qui frappe le premier, qui s'acharne sur un homme à terre. La règle la plus précise gagne, comme chez Ruskin.
5. **La reddition qui engage.** Un bandit à terre peut se rendre. **Il s'arrête vraiment** (l'anti-*Skyrim*). Un bandit sans honneur peut feindre, et c'est un acte que le jeu retient. Le joueur peut épargner ou achever : tout est lu.
6. **La fin du combat est une négociation.** Le chef, en mauvaise posture, peut céder et proposer un marché ; le joueur, battu, tombe dans un événement de mort imminente. Dans les deux cas, on revient au dialogue sans changer de mode.
7. **La mémoire.** Ce qui s'est passé devient des faits : « a épargné », « s'est agenouillé », « a trahi sa reddition ». Les gardes, au retour, en parlent.

**Le minimum pour J5** (à valider) : un concept « le chef exige » ; quatre répliques du joueur ; l'emote s'agenouiller ; une dizaine de répliques du chef et de deux bandits selon la manière de se battre ; un état « rendu » respecté ; une poignée de faits sauvegardés. Pas de rumeurs ni de factions complètes (P2).

### Validation de l'auteur (2026-10-25)

- **La scène et le minimum pour J5 : validés** (« globalement oui », « tu as saisi l'idée »).
- **Point 4, attention à la quantité de barks** : des commentaires pendant le combat, oui, mais sans saturer. À régler dès le départ (fréquence, temps de repos entre deux répliques, priorité aux répliques rares et précises).
- Points 5 (la reddition qui engage) et 6 (la fin en négociation, « par exemple ») : oui.

## Décisions (auteur, 2026-10-25)

- **Personnage** : page blanche (*Skyrim*, *Diablo*).
- **Répliques du joueur** : liste à la *KOTOR*, voix Klatt pour les PNJ. **Muet ou non** : indifférent pour l'auteur (« au choix »).
- **Passer ou accélérer les dialogues, intelligemment** : à prévoir. Référence de l'auteur : *Cyberpunk 2077*, qui le fait bien.
- **Au combat**, l'intention se lit dans les gestes **et** par des gestes explicites (baisser l'arme, emotes).

## Questions ouvertes

1. ~~Le personnage du joueur~~ → page blanche.
2. ~~Les répliques du joueur~~ → *KOTOR* + Klatt ; muet au choix.
3. ~~Au combat~~ → gestes implicites **et** explicites.
4. ~~La scène du chef~~ → validée.
5. Ce que veut dire « intelligemment » pour passer un dialogue (ne jamais sauter un choix ? passer une réplique à la fois ? accélérer la voix ?) : à préciser au moment de la conception technique.
