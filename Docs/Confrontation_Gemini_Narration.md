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

Reconstituées à partir de ce que Gemini reformule ou cite (« ton idée… », « ce que tu viens de lâcher… »). **Confirmées par l'auteur le 2026-10-25** : « c'est bien des trucs que j'ai mis en avant, et c'est surtout comme ça que Gemini en a retenu ». Versées dans [[Plan_EireneNexus]].

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

## 3. Écarts avec le coffre

> **Corrigé le 2026-10-25 après relecture par l'auteur.** La première version présentait sept « contradictions ». Relues phrase à phrase dans le dump, la plupart n'en sont pas : soit Claude prêtait à Gemini une conséquence qu'il n'écrit pas, soit le dump est compatible avec le coffre. Un seul écart réel reste (n° 1, et encore partiel). Le tableau cite le texte du dump pour chaque point.

| # | Ce que Claude avait écrit | Ce que dit le dump | Verdict après relecture |
|---|---|---|---|
| 1 | « Nemesis-like », vengeance contre le joueur : contraire au coffre | « ton Nemesis-like » ; « Son Rival… génère une mission d'assassinat contre toi » | **L'analogie est légitime** : l'auteur présente Eiréné Nexus en le comparant au Nemesis system et en disant que ce n'en est pas un ; le nom a été choisi d'abord pour s'y opposer ([[Plan_EireneNexus]]). Gemini reprend cette présentation. **Seul point à surveiller** : un rival qui lance une mission d'assassinat contre le joueur se rapproche de « se venger du joueur », écarté le 2026-10-12 ([[Plan_FactionAI]]). Si ça naît des intérêts d'une faction (et pas d'une mécanique de vengeance), ça reste dans l'esprit. |
| 2 | ~~Les renforts du thermostat violent « rien ne se crée »~~ | « fait spawner des renforts d'élite **depuis sa capitale** » | **Pas une contradiction.** Le sujet est le **choix** de revenir ou non vers l'équilibre écrit (rester proche de l'authoring, ou laisser vraiment tourner le système). Des renforts qui partent de la capitale sont un déplacement, pas une création. Le principe n° 5 dit seulement que le rappel, quel qu'il soit, se paie dans les réservoirs existants. |
| 3 | ~~Un dictionnaire unique rend les PNJ omniscients~~ | « Toute la mémoire narrative… tient dans UNE SEULE map plate » | La map unique est bien dans le dump ; **l'omniscience, Gemini ne l'écrit nulle part** : c'est une conséquence tirée par Claude. Reste une **remarque de conception** (pas une contradiction) pour `Plan_Dialogue` : avec des rumeurs et des croyances ([[Plan_EireneNexus]]), il faudra savoir *qui* connaît un fait. Gemini en a lui-même l'amorce (`Knows_Orik_Is_Traitor`). |
| 4 | ~~La mort du joueur contredit « on ne meurt pas »~~ | Une seule occurrence : « et s'il te tue, il gagne le titre… » (cas 4) | **Pas une contradiction** : chez nous, « te tuer » déclenche un événement de mort imminente, et l'ennemi gagne une vie (règle 6 de [[Plan_VitalPotential]]). Le titre gagné est compatible. |
| 5 | Eiréné Nexus n'est pas codé | « tu as déjà la brique la plus dure (ton plugin de Graphe… et ton Eriene Nexus) » | Exact, mais **sans conséquence** : simple raccourci de Gemini. |
| 6 | ~~La simulation dans le nœud contredit GraphKit minimal~~ | « ajouter une struct de métadonnées sur les Nœuds de ton plugin de Graphe : `FZoneSimulationPayload` » | **Compatible** : c'est exactement la **charge utile libre** prévue par [[Plan_Graph]]. La simulation reste dans son propre plugin, la donnée voyage dans la charge utile. |
| 7 | ~~Le feu sur le graphe des zones~~ | « la même mathématique sur ton plugin de Graphe » ; « UN SEUL solveur » | Gemini parle surtout de **la même mathématique et du même solveur**, pas forcément du même graphe. Nuance seulement : [[Plan_Fire]] construit son graphe autour du feu, à l'échelle des plaques d'herbe ; un solveur commun peut tourner sur deux graphes d'échelles différentes. |

**Sur les chiffres de Gemini** (auteur, 2026-10-25) : ce sont des ordres de grandeur ; les nombres sont faux, mais l'idée qu'ils défendent tient a priori (un graphe sans rendu se simule très vite, une sauvegarde de faits et de graines est petite).

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

1. ~~Les idées du §1 sont-elles bien les tiennes ?~~ → **Oui** (2026-10-25).
2. ~~L'armure dorée sous la suie lavée par la pluie dans la tranche verticale ?~~ → « C'était juste un exemple, mais ouep, pourquoi pas » : **candidate** pour [[Plan_VerticalSlice]], pas une décision.
3. On commence par `Plan_Dialogue` (Ruskin) pour J5 ? → pas encore répondu.
4. Tags pondérés libres ou axes fixes ? → l'auteur trouve l'idée de Gemini « pas bête » et veut **y réfléchir** : réflexion de Claude dans [[Plan_EireneNexus]] (« Traits : tags libres ou axes fixes ? »).
