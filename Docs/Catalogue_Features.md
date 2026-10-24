# Catalogue des features

> Accueil : [[00 - Accueil]] · Sources : le doc de l'auteur « Plan de jeu UE5 - Systèmes et Features » (Claude Docs, 2026-10-16) et les plans du coffre.

> Statut : 📐 **méthode proposée** (2026-10-17), en attente de validation. Le catalogue est à compléter au fil des idées de l'auteur.

Toutes les features que l'auteur pourrait vouloir, en une ligne chacune, avec leur **profondeur** : jusqu'où elles ont été pensées. On ne code rien qui ne soit pas « creusé ».

## La méthode (idée de l'auteur, 2026-10-17)

> « Si je dis juste que je veux des routes, sans chercher de quelle manière, je ne peux pas faire OpenRoads Designer, alors qu'en vrai c'est simple : il y a plein d'étapes intermédiaires. »

1. **Capturer** : tout noter, même en vrac, même naïf. Un dump est une liste de souhaits, pas une conception.
2. **Creuser** une feature à la fois, avec le gabarit ci-dessous. D'abord **le but final**, à la hauteur des meilleurs outils ou jeux du domaine, puis **les étapes intermédiaires** qui y mènent.
3. **Construire** étape par étape. Chaque étape est utilisable seule et se teste.

Ce que Claude ajoute :
- **Penser loin, construire court.** Le but final sert de boussole : il évite les impasses (une étape qui marche mais qu'il faudra jeter). La construction reste incrémentale, parce qu'un but final rêvé se trompe souvent sur les détails ; on le corrige en route.
- **Le but final se décrit par sa référence pro**, pas par un adjectif. « Des routes » ne dit rien ; « des routes comme OpenRoads : axe en plan, profil en long, profils types, raccord au terrain » dit déjà les étapes.
- **Les briques partagées apparaissent en creusant**, pas avant. Le doc du 16/10 les devinait (A, B, C, D) ; le creusage les confirme ou les corrige.

## Échelle de profondeur

| Niveau | Nom | Ce qui existe |
|---|---|---|
| 0 | Capturé | Une ligne dans ce catalogue |
| 1 | Esquissé | Un plan avec l'intention et des références, mais pas de but final ni d'étapes |
| 2 | Creusé | Le gabarit rempli : but final, étapes, critères |
| 3 | Validé | L'auteur a validé le plan creusé |
| 4 | Codé | Au moins une étape codée (✅ si testée par l'auteur) |

## Gabarit d'une feature creusée

À ajouter dans le `Plan_<Système>.md` de la feature, sous ce titre : `## Feature creusée : <nom>`.

1. **But final** : la version rêvée, en une ou deux phrases, et **sa référence** (un logiciel, un jeu, une publication). Ce que le joueur ou l'auteur en voit.
2. **Ce que la référence fait vraiment** : les concepts du domaine, nommés (pour les routes : axe, profil en long, dévers, assemblage, corridor).
3. **Étapes intermédiaires** : une liste ordonnée. Pour chacune : ce qu'elle apporte seule, et son **jalon testable** (un test automatique, un chiffre, une scène à regarder).
4. **Briques partagées** : ce qui sert aussi ailleurs (lien vers les plans).
5. **Logique / visuel** : ce que Claude peut vérifier seul, ce qui demande l'œil de l'auteur.
6. **Ce qu'on ne fait pas** : les limites assumées.
7. **Questions ouvertes**.

## Le catalogue

`Doc 16/10` = vient du doc de l'auteur du 2026-10-16.

### Briques partagées

| Feature | Plan | Prof. | Note |
|---|---|---|---|
| Attache à un point d'un PNJ qui bouge (brique A) | [[Plan_NPC]] | 1 | Simplifiée par les pièces rigides : un os + un offset |
| Appendice qu'on tire : sangles, cheveux, corde (brique B) | [[Guide_Rope]] | 4 | Plugin `Rope` (PBD) vu marcher ; Cosserat = piste pour le solveur final |
| Horloge musicale, tout sur le beat (brique C) | [[Plan_Rhythm]] | 1 | Doc 16/10 ; un tempo subsystem |
| Perception, blackboard de connaissance, barks (brique D) | [[Plan_NPC]] · [[Plan_FactionAI]] | 1 | Doc 16/10 : illusion de théorie de l'esprit |
| Règle de capacité (qui exécute, qui subit) | [[Plan_NPC]] | 1 | Doc 16/10 ; PNJ vers PNJ jamais codé pour le joueur seul |
| Bus d'événements « X affecte Y » | — | 0 | Doc 16/10 ; forme à trouver |
| Objet graphe générique | [[Plan_Graph]] | 4 | `GraphKit` v0 |
| Jauges | [[Plan_Gauges]] | 1 | Composant de l'auteur |

### Personnage, déplacements, combat

| Feature | Plan | Prof. | Note |
|---|---|---|---|
| Main contrôlée à la souris / manette | [[Plan_3C]] | 2 | Déjà prouvée dans le prototype de l'auteur |
| Outil visuel de filtrage d'entrée | [[Plan_3C]] | 2 | |
| Caméra par modes | [[Plan_3C]] | 1 | |
| Stances (idle, interact, attack, defence, parade) | [[Plan_3C]] | 1 | |
| Glissade, coyote time + tampon de saut, lever le pied, inertie en pente | [[Plan_3C]] | 1 | |
| Sauter sur les ennemis (Mario) | [[Plan_3C]] | 1 | |
| Parkour, wall-run, grimper arbres et murets | [[Plan_3C]] | 0 | Doc 16/10 ; l'escalade du décor est dans le plan |
| Escalader les grands monstres (Shadow of the Colossus, Dragon's Dogma) | [[Plan_3C]] · [[Plan_NPC]] | 1 | |
| Monter, chevaucher | [[Plan_NPC]] | 0 | Animaux montures |
| Diriger la monture en tirant sangles / cheveux | — | 0 | Doc 16/10 ; brique B |
| Contact de l'arme : rebond, glissement | [[Plan_3C]] | 1 | |
| Armes qui se plantent (God of War) | [[Plan_3C]] | 0 | Doc 16/10 |
| Archétypes de combat, pierre-feuille-ciseaux gigogne | [[Plan_3C]] · [[Plan_Loot]] | 1 | |
| Gestes et formes ($1 Recognizer, tracé Skyward Sword) | [[Plan_3C]] | 1 | |
| Interactions : attraper, lancer, mécanismes, bras de levier, emotes | [[Plan_3C]] | 1 | |
| Anti-coincement « code Konami » | [[Plan_3C]] | 1 | |

### PNJ, factions, économie

| Feature | Plan | Prof. | Note |
|---|---|---|---|
| Fiches de PNJ en modules, un squelette, pièces rigides | [[Plan_NPC]] | 1 | |
| Visages expressifs : pièces façon stop motion, sourcils, bouche par la synthèse vocale | [[Plan_Face]] | 1 | « Vague idée à mettre au point » (17/10) ; dialogues à la Skyrim, Deus Ex, Mass Effect |
| Mannequin : assembler un personnage (pièces sur os, profil, ancres) | [[Plan_Mannequin]] | 4 | M1 compilé le 21/10, à tester ; plan validé (+ outils de forme M7) ; base des yeux, bouche, vêtements |
| Éditeur de personnage complet, tête modulaire | [[Plan_Face]] · [[Plan_Mannequin]] | 1 | Demandé par l'auteur (17/10) : mailler une tête entière est complexe, d'où les modules |
| Habiller, déshabiller comme on déballe (manche, gant d'armure, pantalon) | — | 1 | Idée de l'auteur (21/10) : vêtements = meshes par os échangés à la Skyrim ; retirer = ouvrir, glisser le long de l'os (l'os sert d'axe, rayon tissu ≥ rayon membre), séparer puis swap. Cas : rigide (glisse), souple (glisse + accordéon, retournement), passage d'un os à l'autre (coude, main, genou, pied). Commencer bras tendu. Masque de profondeur pour les derniers mm. État de l'art à chercher avant tout aperçu |
| Yeux en nombre libre, placés n'importe où | [[Plan_Eyes]] | 1 | Pas forcément par paire, troisième œil possible ; miroir réglage par réglage |
| Foules, escouades, tokens d'attaque | [[Plan_FactionAI]] | 1 | Pas de Mass |
| Factions organiques, théorie des jeux (Eiréné Nexus) | [[Plan_EireneNexus]] | 1 | |
| Potentiel vital, la vie comme monnaie | [[Plan_VitalPotential]] | 1 | |
| Défaite sans mort | [[Plan_VerticalSlice]] · [[Plan_VitalPotential]] | 1 | |
| Loot : marques, périodes, bris | [[Plan_Loot]] | 1 | Cœur du jeu |
| Synthèse vocale | [[Plan_KlattVoice]] | 3 | Plan validé, pas codé |

### Monde et simulation

| Feature | Plan | Prof. | Note |
|---|---|---|---|
| Terrain en surfaces paramétriques | [[Plan_PatchTerrain]] | 4 | |
| Eau par bassins | [[Plan_Water]] | 4 | |
| Terrain « From Dust » | [[Plan_FromDustTerrain]] | 1 | |
| Feu qui se propage | [[Plan_Fire]] | 1 | Le doc 16/10 le décrit naïvement ; **prochain à creuser ?** |
| Foudre (rythme + feu + lumière) | [[Plan_Rhythm]] | 0 | Doc 16/10 |
| Météo, ciel piloté | [[Plan_Clouds]] | 1 | |
| État du monde, persistance, monde qui guérit | [[Plan_WorldState]] | 1 | |
| Lieux rêvés | [[Plan_DreamPlaces]] | 1 | |
| Niveau d'abstraction (LOA) | [[Plan_LOA]] | 4 | |
| Arbres procéduraux | [[Plan_ProceduralTrees]] | 1 | |
| Routes | [[Plan_CityTool]] · [[Plan_Graph]] | 0 | L'exemple OpenRoads de l'auteur |
| Villes paramétriques | [[Plan_CityTool]] | 1 | |

### Rendu

| Feature | Plan | Prof. | Note |
|---|---|---|---|
| Éclairage maison (OKLab, ombres GDF) | [[Guide_OKLab]] | 4 | |
| Lumières maison | [[Plan_Lights]] | 4 | |
| Culling des lumières | [[Plan_LightCulling]] | 2 | Document de l'auteur |
| Lumières d'ambiance (surfels) | [[Plan_AmbientLights]] | 1 | |
| Forward shading | [[Plan_Forward]] | 4 | |
| Post-process maison | [[Plan_PostProcess]] | 4 | |
| Nuages « John Martin », notre propre Nubis | [[Plan_Clouds]] | 1 | |
| Fog maison | — | 0 | Annoncé le 2026-10-12 |
| Vision scotopique | [[Guide_OKLab]] | 1 | Amorcée dans `Lighting.ush` ; le doc 16/10 la voit en post-process |
| Impact frames | [[Plan_PostProcess]] | 0 | Doc 16/10 |
| Paupières dessinées sans mesh (et leur ombre) | [[Plan_Eyes]] | 1 | Le vrai sujet, pas le globe (auteur, 17/10) |
| Focus : guider le regard | [[Plan_Focus]] | 1 | |
| Transfert de normales | [[Plan_NormalTransfer]] | 4 | |
| Traces GPU | [[Plan_GpuTraces]] | 4 | |

### Outils

| Feature | Plan | Prof. | Note |
|---|---|---|---|
| Composer un mesh de formes simples | [[Plan_KitbashTool]] | 4 | `LayoutKit` v0 |
| Confort d'usage et debug des plugins | [[Plan_EditorUX]] | 2 | En attente de validation |
| Carte des pentes | [[Plan_3C]] | 0 | |
