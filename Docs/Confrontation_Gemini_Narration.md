---
type: confrontation
projet: DWAM
source: conversations de l'auteur avec Gemini (dump 2/2 du 2026-10-25)
mis_a_jour: 2026-10-25
---

# Confrontation : dump Gemini (combat, dialogues, narration, factions, A-Life)

Retour à [[00 - Accueil]] · Références réelles citées : [[References_Precedents]] (section 8).

> **Ce qu'est ce document.** L'auteur a collé huit réponses de Gemini, **sans ses propres messages** : « des trucs moins propres, surtout le résultat de discussions ». Ce document n'est **pas** un plan, et le texte de Gemini n'est pas recopié. On y trouve : (1) les idées de l'auteur qu'on devine à travers les réponses, (2) ce qui existe déjà dans le coffre, (3) ce qui le contredit (**le coffre fait foi**), (4) les affirmations techniques vérifiées ou douteuses, (5) où chaque idée irait.
>
> Ton de Gemini à garder en tête : enthousiaste et vendeur (« production-ready », « 20 minutes avec Claude Code », « 0,001 ms », « < 200 Ko »). Ces chiffres ne sont pas mesurés.

## 1. Les idées de l'auteur qu'on reconnaît (à confirmer)

Reconstituées à partir de ce que Gemini reformule ou cite (« ton idée… », « ce que tu viens de lâcher… »). **À confirmer par l'auteur** avant de les verser dans les plans.

| Idée (formulée par Claude) | Indice dans le dump | Déjà dans le coffre ? |
|---|---|---|
| La scène du chef qui exige « à genoux » ; l'armure dorée couverte de suie que la pluie lave, et le chef qui la reconnaît soudain | Godrick, « I am the king of all that is golden » | L'agenouillement oui ([[Plan_EireneNexus]], [[Plan_VerticalSlice]]) ; **l'armure, la suie, la pluie : nouveau** |
| Un **curseur** par carte ou par moment de l'histoire, entre monde écrit à la main et monde livré à lui-même | « Slider d'autonomie » | Non |
| Ce curseur n'arrête pas le monde : c'est un **thermostat**. En mode « écrit », l'état revient vers la cible de l'auteur à un **petit delta** près ; en mode chaos, plus de force de rappel | « ta vision du Slider… thermostat homéostatique » | Non |
| **Faire tourner en boucle et en accéléré des centaines de graphes**, par graine, avec ou sans influence du joueur, pour voir comment ça dégénère | Citation de l'auteur reprise par Gemini | Proche de la « simulation et si » de [[Plan_VitalPotential]] |
| Le lien avec l'**équilibre de Nash** pour éviter le monopole | « Tu viens de mettre le doigt sur… Nash » | Les coalitions contre le plus fort sont déjà dans le tableau de [[Plan_EireneNexus]] |
| Des factions **imbriquées** (la France → un parti → une branche), une appartenance **graduelle et discrétisée**, une **morale individuelle décorrélée du camp** (le gentil chez les méchants, l'ordure chez les gentils) | « Pardon, je suis parti dans un délire Total War » | Sous-factions hiérarchiques oui ; **paliers d'appartenance et morale séparée : nouveau** |
| **Une faction qui obtient le monopole se resubdivise**, par la distance et les traits | « ce que tu viens de lâcher là » | Les schismes oui (« quand une faction s'étire… ») ; **le monopole qui déclenche la scission : nouveau** |
| Les **marginaux** sont une prise : une porte d'entrée ou un levier de bascule. Exemple : « Jane O'ccide », née là, amoureuse du chef | Nom cité | **Nouveau** |
| Le graphe actuel existe aussi pour rappeler que d'autres systèmes en auront besoin (arbres à la *Far Cry*, feu…) | Citation reprise | Oui ([[Plan_Graph]], « qui en a besoin ») |
| Faire cohabiter missions écrites, missions générées, graphe de zones en distances relatives et un A-Life léger | Question reformulée par Gemini | Oui en partie ([[Plan_DreamPlaces]] : « loin du joueur, le plan du métro » ; [[Plan_FactionAI]]) |
| Un livre français de narratologie au côté « ingénieur », dans la lignée de Barthes | « le bouquin français que tu avais croisé » | Non ; pistes en [[References_Precedents]] section 8 |

## 2. Ce que Gemini propose, rangé par sujet

### 2.1 Combat : détection des coups et dégâts

- **Détection par géométrie, pas par la physique** : des capsules attachées aux pièces (zone, priorité, rayon) ; le coup est testé sur le volume **balayé** entre deux images, par une distance entre segments.
- **Fenêtre d'une image** : quand un même coup touche le bras puis la tête, on garde la zone de plus haute priorité, puis on verrouille ce couple (cible, coup).
- **Paquet de dégâts figé** au départ du coup (les stats de l'attaquant sont copiées : une flèche en vol ne dépend plus de l'archer).
- **Formule en seaux à la *Path of Exile*** : (base + plats) × (1 + somme des « increased ») × produit des « more ».
- **Pont vers les shaders** : le point d'impact, ramené dans l'espace local de la pièce touchée, part vers un render target ou les Custom Primitive Data, pour afficher la blessure.

**Lecture de Claude.** C'est l'architecture classique des jeux d'action, et elle s'accorde avec [[Plan_NPC]] (pièces rigides : une capsule par pièce, c'est naturel) et avec la pipeline des lumières (texture de données lue par les matériaux, [[Plan_Lights]]). Deux adaptations au projet :
- La main est **contrôlée par le joueur** ([[Plan_3C]]) : un coup n'a pas de « N images actives » fixées par une animation. L'arme est dangereuse selon sa **vitesse** et sa stance ; la fenêtre d'une image reste valable.
- La réponse au contact (rebond, glissement, arme plantée, [[Plan_3C]]) a besoin du **point et de la normale** du contact : la distance entre segments donne les deux points les plus proches, donc la normale.

### 2.2 Dialogues et quêtes : faits et règles (Valve)

- Un dictionnaire de **faits** ; des **règles** déclenchées par un événement (« concept ») ; chaque règle a des critères sur les faits ; **la plus spécifique gagne** ; elle produit une réplique, des choix, et des **modifications de faits**.
- Les choix du joueur sont eux-mêmes des règles (sujet de conversation dans le contexte).
- Règles chargées depuis du JSON, pour écrire les quêtes hors de l'éditeur.

**Lecture de Claude.** C'est un **vrai précédent documenté** : Elan Ruskin, GDC 2012 (*Left 4 Dead 2*), que l'auteur a d'ailleurs dans ses onglets. C'est la meilleure candidate pour le **dialogue minimal de J5** ([[Feuille_de_route]]). Une correction de fond, voir 3.3 : pas un dictionnaire unique.

### 2.3 Narration générée : Propp, Greimas, rôles, retournements

- Les missions comme **grammaire** (Propp) réécrite par des « opérateurs de retournement » (le commanditaire est le traître, la cible est déjà morte, l'objet maintient quelque chose en vie).
- Les **rôles** (actants de Greimas) sont distribués à des PNJ **existants** par des requêtes sur leurs traits et leurs relations ; un PNJ peut cumuler deux rôles.
- Le **carré de véridiction** (être / paraître) : chaque entité a des traits **intrinsèques** et des traits **apparents** (filtrés par la suie, le déguisement, le mensonge). Un retournement = une révélation qui fait coïncider les deux.

**Lecture de Claude.** L'être et le paraître existent **déjà** dans le coffre pour les objets : vérité / marquages / connaissance de [[Plan_Loot]] (contrefaçon = marquages ≠ vérité). Les étendre aux personnes et à la surface (suie, mouillé, sang) est cohérent. L'**état de surface** (mouillé, suie, sang) rejoint la pluie de Lagarde ([[References_Precedents]], section 4) : le même nombre piloterait le shader et ce que les PNJ perçoivent. Les rôles distribués à des entités existantes collent aux factions organiques d'Eiréné Nexus.

### 2.4 Monde simulé : verrous, curseur, Monte-Carlo, flux sur graphe

- **Verrous narratifs** à quatre niveaux par nœud ou par PNJ (libre, réservé par une mission générée, protégé « mutable », sanctuarisé), pour que l'A-Life ne casse pas les quêtes écrites.
- **Curseur thermostat** (idée de l'auteur, voir §1) : force de rappel vers la cible de l'auteur, pondérée par (1 − curseur). Ajout de Gemini contre le « syndrome de Sisyphe » : **une action du joueur déplace la cible** (sinon le monde répare tout ce que fait le joueur, comme les avant-postes de *Far Cry 2*).
- **Simulations en accéléré** : sans rendu, des centaines de graines en parallèle ; deux usages : outil d'équilibrage (carte de chaleur des nœuds qui tombent toujours) et **préchauffage** au chargement (garder le meilleur de N mondes selon un score).
- **Un même solveur de flux sur graphe** pour le feu, la corruption et la pression militaire (capacité, conductivité des arêtes, seuil de bascule, épuisement).
- **Sauvegarde en trois couches** : graphe des entités et faits ; graine + tags par région ; différences sur des ancres à identifiant déterministe.

**Lecture de Claude.** La sauvegarde est **exactement** « graine + différences » de [[Plan_WorldState]], avec un ajout utile (ancres à identifiant stable). Le monde sur le graphe loin du joueur est le principe n° 3. Le préchauffage a un précédent réel : la génération de l'histoire de *Dwarf Fortress*. Les verrous et le curseur sont nouveaux et méritent d'être discutés.

### 2.5 Factions : appartenance, morale, marginaux, scission

- Trois dimensions par PNJ : **affiliation** (paliers, imbriquée), **éthique personnelle**, **motif d'adhésion** (contrainte, héritage, réformateur, impunité…).
- Réaction à un acte du joueur = somme des réactions de faction, de sous-faction et d'éthique (l'exemple du prisonnier exécuté devant deux gardes qui réagissent à l'opposé).
- **Trois ancres d'adhésion** : idéologique (proximité des traits avec la faction), géographique, relationnelle. Un **marginal** tient par les ancres géographique ou relationnelle alors que son ancre idéologique est négative : c'est la prise du joueur.
- **Tension de scission** qui monte quand l'ennemi extérieur disparaît, que la faction s'étend loin et que ses traits divergent ; au seuil, un chef périphérique dissonant fonde une sous-faction et chacun choisit son camp.

**Lecture de Claude.** C'est le modèle d'[[Plan_EireneNexus]] (profils de valeurs, proximité = valeurs **et** géographie, regroupement hiérarchique, sécession avec hystérésis) **plus** trois ajouts nets de l'auteur : l'ancre relationnelle, la morale séparée de l'étiquette, la scission déclenchée par le monopole.

## 3. Ce qui contredit le coffre (le coffre fait foi)

1. **« Ton Nemesis-like », vendettas, contrats d'assassinat, titre gagné en tuant le joueur.** Le coffre a tranché (2026-10-12, [[Plan_FactionAI]]) : **rien du Nemesis system, pas de mécanique de vengeance dédiée** ; des élites suivis, des entités à mémoire, des rumeurs. Les rivalités peuvent exister (désir mimétique, théorie des jeux) sans système de vengeance contre le joueur.
2. **« La faction fait apparaître des renforts » pour revenir à l'équilibre.** Contraire au principe n° 5 (**rien ne se crée**) et à [[Plan_VitalPotential]] (somme nulle par acte). La force de rappel du thermostat doit être un **flux** entre réservoirs existants (la faction dépense son potentiel, puise dans la nature, ponctionne un autre acte), pas une création. Bonne nouvelle : un thermostat à somme nulle reste possible, c'est même plus intéressant (le rappel coûte quelque chose à quelqu'un).
3. **Un dictionnaire de faits unique pour tout le monde.** Il rend le monde omniscient : tous les PNJ « savent » que le roi gobelin est mort. Or [[Plan_EireneNexus]] veut de l'**information incomplète** (croyances parfois fausses, rumeurs, espions) et des entités à **mémoire**. Il faut des faits **du monde** (la vérité) et des faits **connus par chaque entité** (ce qu'elle croit), propagés par les rumeurs. Le système de Ruskin interroge d'ailleurs plusieurs sources de faits (le locuteur, le monde, l'événement).
4. **« Si le joueur meurt » / « s'il te tue ».** Principe n° 6 : **on ne meurt pas**, la défaite est un événement de mort imminente. Compatible avec la règle 6 du potentiel vital (l'ennemi gagne une vie), à condition de passer par ces événements.
5. **« Tu as déjà ton plugin de graphe et ton Eiréné Nexus. »** GraphKit existe (v0), Eiréné Nexus **n'est pas codé** (💭 idée).
6. **Mettre toute la simulation dans le nœud du graphe** (`FSystemicGraphNode` avec canaux, verrou, curseur). [[Plan_Graph]] a décidé d'un **socle minimal** : nœuds, arêtes, distance, tags, **charge utile libre**. Chaque système met **sa** charge utile et **ses** règles, dans son propre plugin, sans toucher au socle.
7. **Le feu sur la carte des zones.** [[Plan_Fire]] (idée de l'auteur) construit le graphe **seulement autour du feu**, à l'échelle des plaques d'herbe et des objets. La parenté mathématique (flux, seuil, épuisement) est juste ; le même graphe pour le feu et la guerre, non : ce sont deux échelles.

## 4. Affirmations techniques : vérifiées ou à corriger

| Affirmation | Verdict |
|---|---|
| `FMath::SegmentDistToSegmentSafe` existe | **Vérifié** dans le source 5.8 (`UnrealMathUtility.h`). Mais elle renvoie les **deux points les plus proches**, pas une distance : la distance se calcule ensuite (et donne la normale de contact, utile). |
| « 0,001 ms pour 100 ennemis », « 20 minutes », « < 200 Ko », « 10 Ko de RAM », « 2 ms pour 500 graines » | Non mesuré. Ordres de grandeur plausibles pour certains, à mesurer. |
| La « pénalité de Nash » empêche le monopole | **Abus de langage.** Un équilibre de Nash est un état où personne n'a intérêt à changer seul de stratégie ; il n'empêche pas un monopole par lui-même. Ce que Gemini décrit (« tout le monde tape sur le premier ») est l'**équilibre des puissances** et les **coalitions**, déjà dans [[Plan_EireneNexus]]. Le coût d'entretien croissant avec l'étendue, lui, est une vraie régulation. |
| Granovetter = liens faibles **et** trous structurels | Les trous structurels sont de **Ronald Burt** (1992). |
| La loi de Lanchester « en biologie » | C'est une loi d'**attrition au combat** (1916). Déjà dans [[Plan_FactionAI]] pour les combats abstraits. |
| *Wildermyth*, *Watch Dogs Legion*, *Shadow of War* implémentent Propp | Non vérifié, douteux tel quel. *Wildermyth* génère des histoires par événements ; aucun ne revendique Propp à notre connaissance. |
| L'A-Life de *S.T.A.L.K.E.R.* bridé avant la sortie | Connu dans les grandes lignes (développement long, A-Life réduit). |
| Souriau et ses six fonctions « astrologiques » | Correct dans l'esprit (Lion, Soleil, Terre, Mars, Balance, Lune). |
| Le score de spécificité de Valve | Correct dans l'esprit : la règle qui satisfait le plus de critères gagne. |

## 5. Où chaque idée irait (proposition, à valider)

| Idée | Destination | Priorité proposée |
|---|---|---|
| Faits et règles à la Valve, emotes comme événements, faits par entité | **`Plan_Dialogue`** (nouveau) | **Haute** : J5 de la [[Feuille_de_route]] en a besoin, et il existe un précédent documenté |
| Détection des coups par capsules, zones, paquet de dégâts, pont vers le shader | **`Plan_Combat`** ou chapitre de [[Plan_3C]] | Haute : la scène de J5 a un combat |
| Formule en seaux (increased / more) | [[Plan_Loot]] (modificateurs-verbes) | Moyenne |
| État de surface (mouillé, suie, sang) partagé shader / perception ; être et paraître des personnes | [[Plan_EireneNexus]] + futur `Plan_Weather` | Moyenne : c'est la scène de l'armure, très forte pour la tranche verticale |
| Ancres d'adhésion, paliers, morale séparée, marginaux, scission au monopole | [[Plan_EireneNexus]] | Après confirmation de l'auteur |
| Curseur thermostat (à somme nulle), action du joueur qui déplace la cible, verrous narratifs | [[Plan_EireneNexus]] + [[Plan_VitalPotential]] | Après confirmation |
| Simulations en accéléré (équilibrage, préchauffage) | [[Plan_VitalPotential]] (« simulation et si ») | Outil, plus tard |
| Rôles distribués, opérateurs de retournement, véridiction | `Plan_Narrative` (nouveau) ou [[Plan_DreamPlaces]] | Plus tard (P2) |
| Ancres à identifiant déterministe pour la sauvegarde | [[Plan_WorldState]] | Quand la sauvegarde arrive (J5) |

## 6. Questions à l'auteur

1. Les idées du §1 sont-elles bien les tiennes, et bien formulées ? (Surtout le thermostat, les marginaux, la scission au monopole.)
2. L'armure dorée sous la suie lavée par la pluie : on l'ajoute à la scène de la tranche verticale ?
3. On commence par `Plan_Dialogue` (Ruskin) pour J5 ?
4. Les traits des entités : des tags pondérés libres (ce que dit [[Plan_EireneNexus]]) ou quatre axes fixes (ce que propose Gemini) ?
