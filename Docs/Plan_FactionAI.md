# Plan — Architecture de l'IA : foules, escouades, factions

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_EireneNexus]] · [[Plan_VitalPotential]] · [[Plan_NPC]] · [[Plan_3C]] · [[Plan_DreamPlaces]] · [[Plan_LOA]] · [[Plan_Graph]] · [[Guide_StateTree]] · [[Plan_VerticalSlice]]

> Statut : 💭 **idée**. Le document d'origine (plus bas) a été écrit par une autre session Claude le 2026-10-12 et donné par l'auteur avec cet avertissement : « ils ont été faits un peu hors sol, donc attention ». La section « Confrontation au projet » est de la session qui l'a versé dans le coffre. Rien n'est décidé, rien n'est codé. Aucun point d'API cité dans le document n'a été vérifié dans le source 5.8.

## Précisions de l'auteur (2026-10-12)

- L'autre session n'avait pas le contexte réel du projet (« j'ai pas pu passer 8 h à lui expliquer »). Les écarts ci-dessous viennent de là.
- **« Musou »** = la référence *Dynasty Warriors* : **avoir la possibilité d'afficher beaucoup de PNJ**. Le mot parle de **quantité et d'astuces d'optimisation**, pas d'un changement de genre ni du combat.
- **Pas de Mass Entity : décision de l'auteur.** Ses expériences passées avec Mass ne lui ont « vraiment pas » plu.

Conséquence : l'écart n° 5 (le genre) tombe, l'écart n° 8 est tranché. Ce document est donc à lire comme une **boîte à outils d'optimisation de foule** (tick unique, time slicing, grille spatiale, LOD d'IA et de simulation, tokens, pool), à raccorder au design du coffre, et non comme le design des factions.

## Architecture corrigée : les couches du haut viennent du coffre (2026-10-12)

Demande de l'auteur : « corrige en te basant sur eux [Eiréné Nexus et le potentiel vital], pas sur ce truc d'IA tout nouveau et fait vite fait ». Il les avait cités à l'autre session seulement pour dire que d'autres systèmes feraient ce travail.

Règle de lecture : **les sections 2 et 3 du document d'origine (couche Faction, simulation de guerre) ne font pas foi.** Ce qui fait foi, c'est [[Plan_EireneNexus]] et [[Plan_VitalPotential]]. Le document d'origine reste valable pour les couches du bas (zone, escouade, unité, tokens, performances), comme boîte à outils.

```
Par acte (économie fermée, factions de l'acte)
├─ Eiréné Nexus          qui est avec qui, et pourquoi        → Plan_EireneNexus
├─ Potentiel vital       ce que chaque faction peut dépenser  → Plan_VitalPotential
└─ Graphe de zones       où ça se passe                       → Plan_Graph (GraphKit)
        │
        ▼  zone chargée (LOA : du plan du métro à la carte détaillée)
Commandant de zone → escouades → soldats et officiers         → document d'origine, sections 4 à 8
```

### Ce que le coffre dit, et ce qu'il remplace dans le document

| Sujet | Le document d'origine | Ce qui fait foi (coffre) |
|---|---|---|
| D'où viennent les factions | Une liste de factions, une valeur de relation par paire | Chaque entité a des **tags de valeurs** (goûts et dégoûts). Les factions sont des **regroupements par proximité**, de valeurs **et** de géographie, avec des **sous-factions**. Certaines sont **« en dur »**, posées par le design de l'acte |
| Entrer, sortir, se diviser | Absent (seulement des rancunes locales entre escouades) | **Voulu** : on entre dans une faction, on en sort, il y a des **luttes internes** et des schismes |
| Comment une faction décide | Un score par option (renforcer, raid, fortifier, se venger du joueur, attaquer un faible) | Des **principes de théorie des jeux** : coopérer ou trahir quand on se reverra, céder ou menacer, s'allier contre le plus fort, signaux coûteux, croyances parfois fausses. Les gains se calculent à partir des valeurs (proposition de Claude dans le plan) |
| Mémoire | « Officiers nommés » persistants | Certaines entités ont des **fonctions avancées, dont une mémoire, d'où des rumeurs**. Un **élite** qui a vaincu le joueur est suivi ; un ennemi ordinaire ne l'est pas |
| La ressource | Or, troupes, moral, influence | **Le potentiel vital** : la vie, monnaie cachée. Les factions la **dépensent comme dans un RTS** |
| D'où vient la ressource | Les zones « produisent », les ressources « se régénèrent » | **Rien ne se crée** : somme nulle, **conservée à l'échelle de l'acte**. Seule l'histoire ponctionne un acte au profit d'un autre |
| Tuer | Des pertes, des effectifs qui baissent | Tuer = **absorber** le potentiel vital, avec une perte ; l'équipement se sert avant le joueur ; **la nature a horreur du vide** |
| Le joueur vaincu | Absent | **On ne meurt pas** : un événement de mort imminente (sauvé, capturé, ramené, interrompu). **Quelqu'un paie**. L'ennemi qui l'emporte gagne une vie, surtout au profit de sa faction |
| Le cadre | Un monde, un graphe de zones | **Des actes**, façon *Diablo 2*, chacun avec ses factions et son économie |

### Comment les couches du bas s'y raccordent (proposés par Claude, **validés par l'auteur le 2026-10-12** : « je valide tout »)

- **Matérialiser une escouade, c'est dépenser.** Le « budget de spawn par zone » du document devient une dépense de potentiel vital de la faction. Une faction à sec ne fait plus apparaître personne : c'est la règle 3 du potentiel vital, et un frein naturel à l'emballement.
- **Un combat abstrait conserve le total.** La résolution par rounds loin du joueur (formule de Lanchester du document) peut rester, à condition que ce qu'un camp perd aille quelque part : au vainqueur, à la nature. À vérifier par des tests de conservation, comme l'eau de Watershed.
- **« Qui est hostile à qui » se calcule, puis se met en cache.** La table d'hostilité par zone du document est une bonne astuce d'optimisation ; son contenu vient d'Eiréné Nexus (proximité des valeurs, historique), pas d'une valeur réglée à la main. Les rancunes locales entre escouades du document sont un cas des luttes internes voulues par l'auteur.
- **La mort imminente a besoin du commandant de zone.** Le choix de l'événement dépend de qui est à proximité et de ses relations avec le joueur ([[Plan_VitalPotential]]) : c'est le commandant de la zone chargée qui le sait.
- **Le journal d'événements** du document rejoint les rumeurs : ce que les entités à mémoire se racontent.
- **L'anti-emballement** n'a pas besoin de règles à part : la somme nulle (affaiblir une faction nourrit sa voisine, l'exemple de la Comtesse) et les coalitions contre le plus fort le font déjà.

### Deux autres points où le document a tort (auteur, 2026-10-12 : « il a tort sur ces points »)

Claude les avait d'abord rangés dans « toujours ouvert ». Ils ne le sont pas : le coffre fait foi.

- **Le corps des unités** : c'est [[Plan_NPC]] qui s'applique (un seul squelette, pièces rigides, pas de skinned mesh, transfert de normales). Les instances avec animation en texture et les skeletal meshes du document sont écartés. Il reste un travail d'ingénierie, pas une question de design : afficher beaucoup de PNJ faits de pièces rigides.
- **Les officiers nommés et « se venger du joueur »** : écartés. On s'en tient à [[Plan_EireneNexus]] et [[Plan_VitalPotential]] : élites suivis, entités à mémoire, rumeurs ; pas de mécanique de vengeance dédiée, rien du Nemesis system.

### Ce qui reste réellement ouvert

Seulement les questions ouvertes déjà posées dans [[Plan_EireneNexus]] et [[Plan_VitalPotential]] (où va la perte, ce que le joueur voit, le rôle du joueur).

## Confrontation au projet (première lecture, avant la correction ci-dessus)

### Ce qui s'accorde avec le coffre

- **« Most bang for buck »** : c'est le principe fondateur n° 4.
- **Basculement abstrait ↔ réel** : c'est le principe n° 3 (« loin du joueur, le monde vit sur le plan du métro ») et le LOA ([[Plan_LOA]]).
- **Monde en graphe de zones, façon *Diablo 2*** : déjà noté dans [[Plan_CityTool]]. Le graphe de zones est un candidat direct pour `GraphKit` ([[Plan_Graph]]).
- **State Tree** pour le commandant, l'escouade et les officiers : cohérent avec [[Guide_StateTree]].
- **L'existant de l'auteur** que le document appelle « l'ancienne version BP » des tokens : sans doute `Content/AVOLABO/TokensAI` (`BPAC_SlotProviderComponent`, `PDA_SlotLayout`, `S_CombatSlot`, `E_SlotReservatioNState`) et la tâche `STT_ReserveSlot` de `Content/AVOLABO/New_AI`. À confirmer par l'auteur.

### Ce qui ne colle pas, ou manque

*Points 1 à 3 : traités par l'architecture corrigée. Point 5 : levé par l'auteur. Points 6 et 7 : le document a tort, le coffre fait foi. Point 8 : tranché par l'auteur. Point 4 : simple différence de formulation, le coffre fait foi.*

1. **Eiréné Nexus est appauvri.** Dans le coffre ([[Plan_EireneNexus]]), les factions naissent de **tags de valeurs**, forment des sous-factions par proximité, on y entre et on en sort, certaines entités ont une mémoire (rumeurs), et le tout repose sur la **théorie des jeux**. Le document le ramène à une valeur par paire de factions et à la couverture des zones.
2. **Les ressources.** Le document pose « or, troupes, moral, influence ». Le coffre pose le **potentiel vital** : la vie comme monnaie cachée, à somme nulle par acte ([[Plan_VitalPotential]]). Il faut dire comment les deux se rejoignent, ou lequel remplace l'autre.
3. **« On ne meurt pas : la défaite est une histoire »** (principe n° 6). Le document ne dit rien de la défaite du joueur : capture, dette envers une faction, mort imminente.
4. **A-Life.** Le document écarte « l'A-Life de STALKER » ; le coffre dit « façon A-Life en gros, en plus léger » ([[Plan_DreamPlaces]]). Même intention, formulations opposées : à aligner.
5. **Le genre « musou ».** Le mot n'existe nulle part ailleurs dans le coffre. [[Plan_3C]] décrit un combat en vue subjective avec une main contrôlée, façon *For Honor* ; [[Plan_VerticalSlice]], une scène où un chef exige la soumission. Des centaines d'unités autour du joueur : c'est une direction nouvelle, à confirmer.
6. **Le corps des unités.** [[Plan_NPC]] : pas de skinned mesh, des pièces rigides sur le squelette du mannequin, jointures cachées par le transfert de normales. Le document propose des instances avec animation en texture (AnimToTexture) ou des skeletal meshes. À réconcilier, d'autant que le transfert de normales s'appuie sur le GDF, en retard sur les objets mobiles.
7. **Prudence sur le brevet.** « Officiers nommés qui persistent » et « se venger du joueur » frôlent ce que le coffre s'interdit de reprendre du Nemesis system ([[Plan_EireneNexus]]).
8. **« Pas de Mass Entity »** est présenté comme une contrainte retenue. Décision de l'auteur ou hypothèse de l'autre session : à confirmer.

### Questions à l'auteur

1. Le « musou » est-il la direction du combat de masse, en plus de la main contrôlée, ou une piste explorée ce jour-là ?
2. Potentiel vital ou « or, troupes, moral » : lequel est la ressource des factions ?
3. « Pas de Mass » : c'est ta décision ?
4. Les tokens : on repart de ton `BPAC_SlotProviderComponent`, ou d'une feuille blanche en C++ ?

---

## Document d'origine (autre session Claude, 2026-10-12, non modifié hormis l'en-tête Obsidian retiré)

> ⚠️ Les sections 2 (couche Faction) et 3 (simulation de guerre) ci-dessous **ne font pas foi** : voir « Architecture corrigée » en tête de cette note. Elles sont conservées pour mémoire.

## Architecture IA — Musou à factions

> [!abstract] Principe directeur
> **Most bang for buck** : un maximum de vie apparente pour un minimum de calcul.
> Le joueur juge le monde à ce qu'il voit changer, pas à ce qui est simulé.
> La profondeur ressentie vient de la **boucle de conséquences**, pas de la finesse de simulation.

Contraintes retenues :
- Pas de Mass Entity : acteurs classiques, optimisés.
- Cible : un maximum d'unités (style musou), en suivant le principe « qui peut le plus peut le moins ».
- AIPerception **pas par défaut**, ajoutée seulement là où c'est utile.
- Simulation de monde type **Kenshi / Mount & Blade**, pas A-Life de STALKER (trop fine, trop lourde).

---

### 1. Vue d'ensemble

```
UFactionSubsystem (GameInstanceSubsystem, persistant, sauvegardé)
├─ Eirene Nexus : relations entre factions + couverture des zones
├─ Système de ressources (séparé)
├─ UWarSimulation (tick lent, combats abstraits)
├─ Graphe de zones (données abstraites)
└─ UFactionAgent (un par faction) : stratégie, ressources, intentions
    │
    ▼  zone chargée
ACommander (AInfo + StateTreeComponent, un par zone chargée)
│   Interface entre monde simulé et monde joué
│
└─ ASquad (AInfo + StateTreeComponent)
    │   Membres, formation, ordre en cours
    │
    ├─ ASoldier (APawn, aucun tick) : exécution bête
    └─ AOfficer (ACharacter + AIController + State Tree) : vraie IA

Niveau monde (par level) :
├─ UBattleSubsystem (WorldSubsystem) : registre, LOD de simulation
└─ USoldierManagerSubsystem (TickableWorldSubsystem) : tick unique des soldats
```

#### Flux

- **Les ordres descendent** : Faction → Commandant → Squad → Unités.
- **Les événements remontent** (delegates) : mort, pertes, base détruite, officier tué.
- **Le joueur et l'IA utilisent la même interface d'ordres** (`FUnitOrder`). L'IA n'est qu'un joueur qui clique tout seul.

---

### 2. Couche Faction (stratégique, persistante)

#### `UFactionSubsystem` — `UGameInstanceSubsystem`
- Vit **hors des niveaux** (les zones se chargent et se déchargent).
- Contient les `UFactionAgent`, le graphe de zones et les armées abstraites.
- Héberge (ou référence) **Eirene Nexus** et le **système de ressources**.
- Entièrement **sérialisé dans la sauvegarde**.

#### Eirene Nexus — relations et couverture
> [!info] Système maison
> Nommé d'après **Eirene** (Εἰρήνη), déesse grecque de la paix.

Responsabilités :
- **Relations entre factions** : une valeur par paire (hostile ↔ allié), qui évolue avec les événements (attaque subie, ennemi commun, actions du joueur).
- **Couverture des zones** : quelle faction couvre ou contrôle quelle zone, zones disputées.

Hors périmètre : les ressources (système séparé).

Consommateurs :
| Système | Ce qu'il demande à Eirene Nexus |
|---|---|
| `UFactionAgent` | Relations et territoire, pour scorer ses décisions |
| `UWarSimulation` | Qui est hostile à qui quand deux armées se croisent ; mise à jour de la couverture après un combat |
| `ACommander` | Qui contrôle la zone chargée, qui est ennemi localement |
| Squads / unités | Hostilité pour le ciblage (via cache, jamais de requête par unité et par frame) |

Événements entrants :
- Zone prise ou perdue → mise à jour de la couverture.
- Attaque, trahison, aide, action du joueur → ajustement des relations.

> [!tip] Performance
> Les unités ne questionnent jamais Eirene Nexus directement. Le Commandant met en cache une table d'hostilité locale, rafraîchie sur événement, qui intègre aussi les modificateurs locaux (voir section 4, Relations locales).

#### Système de ressources (séparé)
- Or, troupes, moral, influence, production des zones.
- Consulté par les `UFactionAgent` ; alimenté par la couverture des zones (une zone couverte produit pour sa faction).

#### `UFactionAgent` — `UObject`
- Consomme le **système de ressources** : or, troupes, moral, influence.
- Lit son **territoire** dans Eirene Nexus : zones couvertes.
- Pense **lentement** : toutes les quelques secondes de jeu, ou sur événement (changement de zone du joueur, perte de base…).
- Décide par **scoring utilitaire** : chaque option reçoit un score selon l'état du monde, la meilleure l'emporte.
  - Renforcer une zone
  - Lancer un raid
  - Fortifier
  - Se venger du joueur
  - Attaquer une faction affaiblie
- Produit des **intentions**, jamais des ordres d'unités :
  - « Zone 4 : tenir, budget 300 »
  - « Raid sur la zone 7 »

#### Influence sur les squads
La faction **paramètre** ce qui sera matérialisé, elle ne contrôle pas directement :
- Composition et nombre (**budget de spawn par zone**)
- Doctrine / agressivité (défensif, embuscade, chasse au joueur)
- Équipement / niveau selon les ressources dépensées
- Présence d'officiers

#### Données de zone
| Champ | Rôle |
|---|---|
| Faction couvrante | Territoire (source : Eirene Nexus) |
| Garnison abstraite | Effectif, force, moral |
| Ressources | Production, valeur stratégique |
| Connexions | Arêtes du graphe, temps de trajet |
| Intention active | Ce que la faction veut faire ici |

---

### 3. Simulation de guerre (factions entre elles)

#### Relations
- Fournies par **Eirene Nexus** (voir section 2).
- La WarSimulation lit les hostilités et renvoie les résultats de combat (zones prises ou perdues, agressions) à Eirene Nexus.

#### Armées abstraites — `FArmyData`
```cpp
USTRUCT()
struct FArmyData
{
    GENERATED_BODY()

    FName FactionId;
    int32 Strength;          // effectif
    float Power;             // force calculée depuis les DataAssets
    float Morale;
    int32 CurrentZone;
    int32 DestinationZone;
    float TravelProgress;    // 0..1 sur l'arête du graphe
    // Composition : réf. vers DataAssets + quantités
};
```
- Déplacement **sur le graphe**, avec un simple temps de trajet. Aucun pathfinding hors zone chargée.

#### `UWarSimulation`
- Tick lent en temps de jeu.
- Déplace les armées, détecte les rencontres et résout les combats.

#### Résolution des combats
- Formule **force contre force** de type Lanchester : les pertes sont proportionnelles à la puissance adverse.
- Modificateurs : défense de base, moral, terrain, officiers présents.
- Résolution **par rounds**, pas instantanée : le joueur peut arriver au milieu d'un combat.

> [!important] Cohérence
> Les stats abstraites dérivent des **mêmes DataAssets** que les vraies unités.
> Le résultat simulé doit ressembler à ce qui se passerait si le combat était joué.

#### Temps écoulé hors de vue
Pas de simulation tick par tick pour une zone non visitée : au retour du joueur, on **calcule l'écoulement d'un coup** (N rounds de combat, régénération des ressources).

#### Garde-fous de design
- [ ] **Anti-snowball** : coût d'entretien des armées, malus de surextension, coalitions contre la faction dominante.
- [ ] **Seed déterministe** : debug et sauvegardes simplifiés.
- [ ] **Journal d'événements** (« la zone 5 est tombée aux mains de X ») : debug et information du joueur.

---

### 4. Couche Zone / Bataille (tactique locale)

#### `ACommander` — `AInfo` + `StateTreeComponent`
- Un par zone chargée.
- Lit l'état de la zone dans le `UFactionSubsystem` et **construit les squads** à partir de ça.
- Gère localement : bases, moral par secteur, renforts, envoi des officiers.
- Tick lent (0,5 à 1 s).
- Ne bouge **jamais** une unité directement.

#### Relations locales (coopération et infighting)

> [!important] Côté IA, pas Eirene Nexus
> Eirene Nexus donne la relation **de fond**. Les ajustements locaux sont gérés ici, par le Commandant.

**Relation effective = relation globale (Eirene Nexus) + modificateurs locaux**
- Modificateurs **temporaires**, qui décroissent avec le temps.
- La **table d'hostilité en cache**, lue par les unités, est calculée à partir des deux.

Granularité :
| Niveau | Exemple | Statut |
|---|---|---|
| Faction × faction (dans la zone) | Trêve locale contre un ennemi commun, rupture locale entre alliés | Oui |
| Squad × squad (rancune) | Le squad A frappé par le squad B devient hostile à B seulement | Oui |
| Unité × unité | — | **Jamais** (trop fin, imperceptible) |

Déclencheurs d'**infighting** :
- Tir ami / dégât de zone sur un squad voisin, d'où une rancune.
- Officier tué, moral effondré : squad indiscipliné.
- Traits de faction : rivalités internes, opportunisme contre un allié affaibli.

Déclencheurs de **coopération** :
- **Ennemi commun présent** (joueur ou tierce faction) au-dessus d'un seuil de menace : trêve locale.
- Seuil réglable, comme levier de difficulté : plus le joueur est fort, plus vite les ennemis se liguent.

**Remontée** : un incident local marquant (trahison, massacre) peut être envoyé à Eirene Nexus et dégrader la relation globale. Un seuil filtre les incidents mineurs.

#### Basculement abstrait ↔ réel
- **Entrée du joueur** : matérialisation des deux camps depuis leur état actuel (effectifs restants, moral). Le combat continue en temps réel.
- **Sortie du joueur** : on « gèle » le résultat réel (survivants, pertes) et la zone repasse en abstrait.

#### `UBattleSubsystem` — `UWorldSubsystem`
- Registre local : factions, bases, squads.
- **LOD de simulation** : décide quels squads sont réels ou abstraits.
- Résout les combats abstraits entre squads lointains de la même zone.

> [!tip] Le plus gros gain
> 2000 soldats « sur la carte », 200 à 300 réellement simulés autour du joueur.

#### `ABattleBase`
- Capturable.
- Point de spawn des renforts.

---

### 5. Couche Squad

#### `ASquad` — `AInfo` + `StateTreeComponent`
- Membres, formation, ordre en cours (`FUnitOrder`).
- Calcule **un seul chemin** pour le centre du groupe (`UNavigationSystemV1::FindPathSync`).
- Attribue à chaque membre un **slot de formation** (offset relatif au centre).
- Gros groupes vers une même destination : **flow field**.
- Agrège l'état et le remonte : arrivé, en combat, X % de pertes.
- **État abstrait** quand il est loin : effectif, force, position.

#### `FUnitOrder`
```cpp
UENUM()
enum class EUnitOrderType : uint8
{
    None, Move, Attack, Hold, Gather, Retreat
};

USTRUCT(BlueprintType)
struct FUnitOrder
{
    GENERATED_BODY()

    UPROPERTY() EUnitOrderType Type = EUnitOrderType::None;
    UPROPERTY() TWeakObjectPtr<AActor> Target;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() bool bQueued = false;
};
```

---

### 6. Couche Unité

#### Deux classes d'IA

| | Soldat | Officier |
|---|---|---|
| Base | `APawn` | `ACharacter` |
| Contrôleur | Aucun, ou sans tick | `AIController` |
| Logique | Enum C++ | State Tree |
| Tick | Aucun (manager central) | Normal |
| Perception | Grille spatiale | AIPerception si besoin |
| Nombre | Centaines | Poignée |

#### `ASoldier` — composition, aucun tick

| Composant | Rôle |
|---|---|
| `USquadMemberComponent` | Appartenance, slot de formation |
| `UUnitMovementComponent` | Mouvement custom léger |
| `UCombatComponent` | PV, attaque, réception des coups |
| `UUnitStateComponent` | Machine à états (enum) |

> [!note] Composition sans tick
> Les composants sont du **stockage et des points d'entrée**.
> Toute la logique de mise à jour tourne dans `USoldierManagerSubsystem`, qui itère sur eux.

#### `USoldierManagerSubsystem` — `UTickableWorldSubsystem`
- **Tick unique** de tous les soldats, sur des tableaux.
- **Time slicing** : une fraction des soldats mise à jour par frame.
- **LOD d'IA** : loin ou hors écran, mise à jour moins fréquente.
- **Grille spatiale** (spatial hash) : voisins, ciblage, séparation.
- **Pool de soldats** : spawn et despawn réutilisés.

#### Officiers nommés
- Peuvent **persister** dans le `UFactionSubsystem` : vivant ou mort, niveau, faction.
- Une entrée de table, pas une simulation.

---

### 7. Système de tokens d'attaque

> [!info] Principe
> La **cible** possède N tokens. Un attaquant doit en obtenir un pour frapper ; les autres tournent autour en attendant.
> C'est le cercle d'ennemis typique du musou, pour un coût quasi nul.

#### `UAttackTokenComponent`
- Sur le **joueur** et les **officiers**.
- Sur les **soldats** aussi (version légère) ou **un token par squad**, pour le combat faction contre faction.
- Logique en **C++**, réglages exposés en `UPROPERTY(EditAnywhere)`.
- Le nombre de tokens sert de levier de difficulté.

#### Checklist (audit de l'ancienne version BP)
- [ ] **Libération garantie** : mort, repoussé, changement de cible, retour au pool. (Bug n°1 : tokens perdus, plus personne n'attaque.)
- [ ] **Timeout** : un porteur qui ne frappe pas perd son token.
- [ ] **Priorité** : le plus proche ? l'attente la plus longue ? officiers d'abord ?
- [ ] **Types d'attaque** : pool unique ou pools séparés (mêlée / distance / lourde).
- [ ] **Cooldown** après rendu.

> [!tip] Récupérer le BP
> Sélectionner les nodes, faire Ctrl+C, puis coller : Unreal copie les nodes en texte lisible.

---

### 8. Performances (acteurs, sans Mass)

#### Mouvement
- **Pas de `CharacterMovementComponent`** pour les soldats.
- Mouvement custom : avancer vers le prochain point, puis projection navmesh ou trace sol.
- Évitement : **séparation via la grille spatiale**, pas Detour Crowd.

#### Ciblage et collisions
- Pas d'AIPerception ni d'overlaps pour les soldats.
- Pas de collision physique entre unités.

#### Rendu (souvent le vrai goulot)
- **Instanced Static Meshes + Vertex Animation Textures** (plugin AnimToTexture).
- Si skeletal : animation sharing, URO, anims coupées hors écran.

#### Mesure
- **Unreal Insights** dès le début.
- Stress tests à 500 puis à 1000 unités, avant d'empiler le gameplay.

> [!warning] Limite
> Au-delà de quelques milliers d'unités simulées en réel, on finit par réécrire une partie de Mass.
> Le LOD de simulation est là pour ne jamais y arriver.

---

### 9. Ce qu'on ne simule jamais

- Des individus persistants hors de vue : un soldat hors zone n'est qu'un chiffre dans un effectif.
- Des besoins ou horaires individuels (manger, dormir, commercer).
- Des inventaires de PNJ ordinaires.
- Du pathfinding hors zone chargée.

---

### 10. Ordre d'implémentation

1. [ ] **SoldierManager + grille spatiale + tokens (C++)** : le ressenti musou.
2. [ ] **Squad + Commandant de zone** : la bataille locale qui vit.
3. [ ] **FactionAgent + graphe de zones** : stratégie et ressources.
4. [ ] **WarSimulation + basculement abstrait ↔ réel** : le monde qui bouge sans le joueur.

Chaque étape est jouable seule : tester et ajuster avant d'empiler la suivante.
