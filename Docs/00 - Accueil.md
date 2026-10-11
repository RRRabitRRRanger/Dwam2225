---
type: accueil
projet: DWAM
mis_a_jour: 2026-10-12
---

# Coffre de Claude — DWAM (Dwam2225)

Coffre de travail de Claude pour le projet Unreal `Dwam2225`. Il contient les plans, le suivi et les décisions techniques.

- **Versionné avec le code** : un plan et le code qui l'implémente avancent ensemble, sur la même branche.
- **Pour l'ouvrir** : dans Obsidian, *Open folder as vault* sur ce dossier `Docs/`.
- **Coffre de l'auteur** (vision, piliers, brainstorming) : `D:\Nos Documents\AI Engineering DWAm`. **Lecture seule pour Claude.**

## Pour toute session Claude qui arrive ici

L'auteur confie ce coffre aux sessions Claude (2026-10-02) : c'est à elles de le tenir propre et à jour.

1. **Lire cette note en entier** avant de travailler, puis le plan du système concerné.
2. **Un plan par système** (`Plan_<Système>.md`), relié à cette note et à ses voisins par des liens `[[...]]`.
3. **Mettre à jour** le tableau « Systèmes » et le **journal** en fin de session (une ligne datée).
4. **Ne pas réécrire l'histoire** : corriger une erreur, oui ; effacer une décision de l'auteur, non. Marquer ce qui est remplacé (~~barré~~ + nouvelle décision datée).
5. **Le coffre de l'auteur** (`D:\Nos Documents\AI Engineering DWAm`) se lit, mais **on n'y écrit jamais**.
6. **Ne rien inventer** : ce qui est inconnu est marqué `À COMPLÉTER`, et on pose la question.
7. **L'auteur dicte souvent au micro** : interpréter les fautes de transcription avec bienveillance, demander en cas de doute.

## 🔥 Principes fondateurs (en lettres de feu)

Ce que l'auteur a posé et qui guide tout le reste. À relire avant de concevoir quoi que ce soit.

1. **Le graphe porte l'identité, la géométrie porte la variation.** Un lieu se reconnaît à ses liens et à ses rôles, pas à sa forme. ([[Plan_DreamPlaces]], [[Plan_Graph]])
2. **On se souvient d'un lieu par ses voies, ses limites, ses quartiers, ses nœuds et ses repères** (Kevin Lynch). Le cœur ne bouge pas, les feuilles changent. ([[Plan_DreamPlaces]])
3. **Un même lieu existe à plusieurs échelles** : le plan du métro, la carte détaillée, le récit, le jeu. **Loin du joueur, le monde vit sur le plan du métro ; près de lui, sur la carte détaillée.** ([[Plan_DreamPlaces]]) C'est le **LOA**, le niveau d'abstraction : du JRPG au hyper-réaliste, le graphe se développe quand on descend. ([[Plan_LOA]])
4. **On ne simule que ce que le joueur peut sentir** (« bang for buck »). ([[Plan_EireneNexus]])
5. **Rien ne se crée** : la vie et l'eau circulent, à somme nulle. ([[Plan_VitalPotential]], [[Plan_Water]])
6. **On ne meurt pas : la défaite est une histoire**, et quelqu'un paie toujours la vie qui sauve. ([[Plan_VitalPotential]])
7. **Pas de hiérarchie d'âge, et la valeur n'est pas la qualité** : un objet est plus ou moins pertinent selon le contexte. ([[Plan_Loot]])
8. **Gagner plutôt que perdre** : le bris est une récompense, pas une punition. ([[Plan_Loot]])
9. **L'équilibre vient des cycles** (pierre-feuille-ciseaux gigogne), pas de chiffres réglés à la main. ([[Plan_Loot]])
10. **L'arme est un filtre** : sa maniabilité filtre le geste de la main. ([[Plan_3C]])
11. **Du contrôle, pas de physique subie** : pas d'active ragdoll. ([[Plan_3C]])

## Systèmes

| Système | Plan | État |
|---|---|---|
| Terrain en surfaces paramétriques | [[Plan_PatchTerrain]] | 🔧 Plugin `PatchTerrain` codé. Étape 1 testée ; maillage adaptatif, rampe à raccord auto, value noise et lattice compilés, pas encore testés |
| Eau par bassins | [[Plan_Water]] · [[Guide_Watershed]] | 🔧 Plugin `Watershed` : hiérarchie + Fill–Spill–Merge testés (5 tests automatiques, 1 km² en 84 ms), sources, pluie, lacs en tuiles. Pas encore testé dans une map ; cavernes 3D et eau en mouvement pas commencées |
| Synthèse vocale | [[Plan_KlattVoice]] | 📐 Plan validé, anglais d'abord, pas codé |
| GPUTrace (traces sur la géométrie visuelle) | [[Plan_GpuTraces]] · [[Guide_GPUTrace]] | 🔧 Plugin `GPUTrace` compilé, pas testé : moteur CPU utilisable tout de suite (nœud « Async GPU Trace », debug, agent Bug) ; moteur GPU à monter dans Niagara (guide) |
| Transfert de normales + snake | [[Plan_NormalTransfer]] · [[Guide_NormalTransfer]] | 🔧 `NormalTransfer.ush` validé par compilation HLSL hors moteur ; `Snake Toy` compilé ; matériaux à monter (guide) |
| Post-process maison (exposition, Hable, Tony McMapface, gamma, menu) | [[Plan_PostProcess]] · [[Guide_PostProcess]] | 🔧 `PostProcess.ush` validé par compilation ; composant `Dwam Sun Glare` (éblouissement Wind Waker) compilé ; convertisseur de LUT testé. Matériau, MPC, menu et sauvegarde à monter (guide) |
| Corde | [[Plan_NiagaraRope]] · [[Guide_Rope]] | 🔧 Plugin `Rope` (CPU ; Niagara reporté, voir le plan) : solveur testé (4 tests automatiques), enroulement autour d'un objet. Pas encore vu dans l'éditeur |
| Tranche verticale | [[Plan_VerticalSlice]] | 📐 Brouillon à construire avec l'auteur : prouver le combat à la main, la défaite sans mort, un monde qui réagit |
| 3C : personnage, caméra, contrôles | [[Plan_3C]] | 📐 Proposition, en attente de validation : main contrôlée à la souris / manette (à la *For Honor*), filtrage d'entrée + outil visuel, caméra par modes, stances |
| State Tree (référence) | [[Guide_StateTree]] | ✅ Guide écrit à partir du source 5.8 et des State Trees de l'auteur : briques, sélection, transitions, liaisons, événements, schémas, nœuds en Blueprint et en C++, usages dans DWAM |
| Fiches de personnage des PNJ | [[Plan_NPC]] | 💭 Idée : fiche en modules (corps, perception, aggro, combat, esprit, valeurs, jauges, potentiel vital) ; un seul squelette (le mannequin) déformé en bipèdes et quadrupèdes ; pas de skinned mesh : pièces rigides + transfert de normales ; animaux montures et escaladables ; existant : `PDA_ProfileSheet_*` |
| Jauges (composant réutilisable) | [[Plan_Gauges]] | 💭 `Comp_Gauges` de l'auteur + tags `Gauge.*` ; avis noté, pas à coder pour l'instant |
| Eiréné Nexus : factions et théorie des jeux | [[Plan_EireneNexus]] | 💭 Idée : factions organiques (tags de valeurs, regroupements, sous-factions, factions « en dur » par acte), théorie des jeux, histoires émergentes ; nom opposé à Nemesis, sans reprendre ses mécaniques |
| Potentiel vital (la vie comme monnaie) | [[Plan_VitalPotential]] | 💭 Idée : monnaie cachée, somme nulle par acte (comme un jeu incrémental en fond), factions qui dépensent comme dans un RTS, résurrection par potions et autels, magie « boule de neige » ; lié à Eiréné Nexus |
| Lieux rêvés : du hasard reconnaissable | [[Plan_DreamPlaces]] | 💭 Idée : des lieux qui varient sans perdre leur identité (le village d'enfance, le « Paris rêvé ») ; le graphe porte l'identité, la géométrie la variation |
| Loot : marques, périodes, éditions | [[Plan_Loot]] | 💭 Idée : principes de l'auteur posés (pas de hiérarchie d'âge, valeur ≠ qualité, âge d'or = signatures, contrefaçons implicites) ; durabilité et bris façon BOTW, pièces récupérées, montée en niveau, gemmes |
| Feu qui se propage | [[Plan_Fire]] | 💭 Idée : graphe de nodes construit autour du feu (idée de l'auteur), herbe par plaques |
| État du monde et persistance | [[Plan_WorldState]] | 💭 Idée : graine + différences, monde qui guérit ; pas tranché |
| Éclairage maison (DwamLighting) | [[Guide_OKLab]] | 🔧 Plugin créé (mapping `/Plugin/DwamLighting`). `OKLab.ush` (conversions, gamut analytique validé numériquement) et `Lighting.ush` (diffus OKLab, spéculaires, garde-fou de normal map) validés par compilation ; pas encore testés en éditeur. Lumières : voir la ligne suivante |
| Lumières : collecte, packing, envoi aux matériaux | [[Plan_Lights]] · [[Guide_Lights]] | 🔧 Plan validé par l'auteur (2026-10-11) : composant maison `Dwam Light` (pas de lumières d'Unreal « fantômes »), render target 256 × 1 rempli sur le render thread par une Scene View Extension, 2 pixels par lumière, positions relatives à la caméra, somme en RGB, soleil à part. Point, spot et rect codés ; compilé ; 3 tests du packing réussis ; `Lights.ush` validé hors moteur. **Pas encore vu dans l'éditeur** : render target à créer (guide). Restent le fondu de sortie du budget et les ombres par lumière |
| Culling des lumières (Z-binning) | [[Plan_LightCulling]] | 📐 Document de l'auteur (2026-10-11) : Z-bins 1D + masques de tuiles 2D, construits sur GPU dans une Scene View Extension, lus par les matériaux via des render targets ; sept étapes. Budget annoncé : 32 à 64 lumières directes. Pas codé |
| Lumières d'ambiance (rebond, couleur qui déteint) | [[Plan_AmbientLights]] | 💭 Idée de l'auteur : un type de lumière pour l'ambiance, des plans émissifs (cubes) plutôt que des points, répartis en quadtree, pas en screen space ; « un citron dans une serviette met du jaune ». **Méthode retenue : surfels** (décision du 2026-10-11) ; étapes proposées, à valider |
| Focus : un point d'attention qui guide le regard | [[Plan_Focus]] | 💭 Idée de l'auteur : une « sphère » de focus à moitié automatique, à moitié authored (volumes), qui module subtilement shaders et particules ; points d'intérêt par component ou user data, triés par score (le soleil en a un) ; rebond du soleil jusqu'au joueur à la sortie d'une caverne |
| Passage en forward shading | [[Plan_Forward]] | 📐 **Décision de l'auteur (2026-10-12)** : passer en forward dès que possible. Plan écrit (effets vérifiés dans le source 5.8, réglages proposés, contrôles) ; quatre questions en attente ; `DefaultEngine.ini` pas encore modifié |

**Légende** : ✅ fait · 🔧 en cours · 📐 conçu · 💭 idée

## Outils Unreal (pas strictement DWAM)

Des outils dont l'auteur a besoin dans Unreal, rangés ici pour ne pas les perdre. Source principale : son doc « Idées d'outils Unreal » (Claude Docs, 2026-10-05).

| Outil | Plan | État |
|---|---|---|
| Outil ville : BP Anchor, édition façon Blender | [[Plan_CityTool]] | 💭 Idée. Paramétrique emboîté (maison, quartier, ville), carrefours par modèles, simplification plutôt que jumeau numérique. Existant oublié, `Plugins/CitySplineTools` : une surface triangulée à partir d'une spline, peu de « route » ; **à part, voire à virer** (auteur) |
| Composer un mesh à partir de formes simples (BP_Edition) | [[Plan_KitbashTool]] | 🔧 **v0 codée** : `LayoutKit` (sur `GraphKit`), boîte arrondie + entrée, fixe / étirable, 5 tests ; [[Guide_LayoutKit]]. Idée d'origine : paramétrique comme une mise en page d'UI ; servirait aussi aux créatures et aux armes en pièces |
| Arbres procéduraux et dynamiques | [[Plan_ProceduralTrees]] | 💭 Idée : Space Colonization, timidité des cimes, destruction, repousse |
| Terrain « From Dust » (2,5D, GPU) | [[Plan_FromDustTerrain]] | 💭 Idée : automates cellulaires sur GPU ; autre paradigme que PatchTerrain et Watershed |
| LOA : niveau d'abstraction | [[Plan_LOA]] | 🔧 **v1 codée** dans le plugin `LOA` de l'auteur, appliquée à LayoutKit (curseur LOA, la maison se replie jusqu'à « Pokémon »). Idée de l'auteur : du JRPG au hyper-réaliste, le graphe se développe quand le LOA baisse ; première application : LayoutKit |
| Objet graphe générique (remplaçant de la spline) | [[Plan_Graph]] | 🔧 **v0 codée** : `GraphKit` (nœuds nommés, arêtes avec distance, tags, charge utile), 3 tests. Idée : socle commun à plusieurs projets (routes, chaufferie, arbres, feu, factions) ; données + couche spatiale + vues. Le proto `UI/EUW_Graph` (à garder, « c'est cool ») en serait la vue abstraite |

## Liens entre systèmes

- **[[Plan_PatchTerrain]] → [[Plan_Water]]** : le terrain fournit la hauteur du sol et la pente ; l'eau n'en dépend pas.
- **[[Plan_PatchTerrain]] → DwamLighting** : ombres du terrain (champ de distances, horizon map), POI cuits avec MDF.
- **[[Plan_Water]] → DwamLighting** : même texture de hauteur ; même chemin GPU (RDG) que le culling de lumières ([[Plan_LightCulling]]).
- **[[Plan_GpuTraces]] ↔ [[Plan_PatchTerrain]]** : seules les traces HW Ray Tracing voient le terrain (meshes dynamiques absents du GDF).
- **[[Plan_NiagaraRope]] → [[Plan_GpuTraces]] / [[Plan_PatchTerrain]]** : la corde collisionne avec le terrain via sa texture de hauteur ou des traces.
- **[[Plan_Lights]] → [[Plan_LightCulling]]** : la brique des lumières fournit la texture (et la boucle brute-force de référence) ; le culling la trie et la distribue aux pixels.
- **[[Plan_Focus]] → [[Plan_AmbientLights]] → [[Plan_LightCulling]]** : le focus choisit les points d'intérêt, les lumières d'ambiance en font des émetteurs, le culling les distribue aux pixels.
- **[[Plan_NormalTransfer]] → DwamLighting** : la bibliothèque `NormalTransfer.ush` vit dans le plugin `DwamLighting` ; elle alimentera la normale de l'éclairage maison.
- **[[Plan_GpuTraces]] → [[Plan_NormalTransfer]]** : le snake rampe avec des traces complexes CPU ; il pourra passer sur GPUTrace.
- **Matériaux du terrain** (étape « Matériaux » de [[Plan_PatchTerrain]]) : l'orientation par cellule Voronoi est réutilisée par l'eau (sens du courant).

## Références à creuser

Liens donnés par l'auteur, pas encore rattachés à un plan.

| Référence | Lien | Pour quoi |
|---|---|---|
| *Surface-Stable Fractal Dithering Explained* (vidéo) | https://www.youtube.com/watch?v=HPqGaIMVuLs | Ajouté le 2026-10-11. Usage dans DWAM : `À COMPLÉTER`. Voisin possible (lecture de Claude) : les motifs filtrés de `Patterns.ush` ([[Guide_OKLab]]) |
| *UE5: Materials (Ep.17 How to combine multiple normal maps (BlendAngleCorrectedNormals))* (vidéo) | https://www.youtube.com/watch?v=5tN-DPAeW3c | Ajouté le 2026-10-11. Auteur : « pour mélanger simplement des normales ». Voisin possible (lecture de Claude) : [[Plan_NormalTransfer]] |
| Exemple de *rain drip shader* (vidéo) | https://www.youtube.com/watch?v=6hqiBHAHryg | Ajouté le 2026-10-11. Auteur : « un exemple de rain drip shader ». Titre exact de la vidéo : `À COMPLÉTER` |
| *Experimental Niagara-based wave propagation shader - WIP #1*, par Ghislain Girardot (vidéo) | https://www.youtube.com/watch?v=ILRxng6XaAI | Ajouté le 2026-10-11. Usage dans DWAM : `À COMPLÉTER`. Voisin possible (lecture de Claude) : l'eau en mouvement de [[Plan_Water]] |
| *UE5 Intercepting Moving Targets (Math Formula)*, par Royal Skies (vidéo) | https://www.youtube.com/watch?v=kGbSXcdI7X0 | Ajouté le 2026-10-11. Usage dans DWAM : `À COMPLÉTER`. Voisin possible (lecture de Claude) : le combat des PNJ ([[Plan_NPC]]) |
| *Grinding Unreal Engine 5 C++ Tutorial* (vidéo) | https://www.youtube.com/watch?v=wAiPJ23UTqU | Ajouté le 2026-10-11. Chaîne dictée « ohn isril » (nom exact : `À COMPLÉTER`). Usage dans DWAM : `À COMPLÉTER`. Voisin possible (lecture de Claude) : les déplacements de [[Plan_3C]] |

## Conventions

- Un plan par système, en français ; termes Unreal en anglais.
- Distinguer **décision** (validée par l'auteur, datée) et **proposition**.
- Avant un changement important : plan ici, puis validation de l'auteur.
- Commits sur une branche dédiée, jamais directement sur `master`.

## Journal

| Date | Ce qui a été fait |
|---|---|
| 2026-10-01 | Exploration du projet (Blueprints, State Trees, shaders HLSL). Plans KlattVoice et PatchTerrain. Plugin `PatchTerrain` : canevas, modificateurs, expressions, tampons. |
| 2026-10-02 | Maillage adaptatif, value noise, rampe à raccord auto, lattice. Plan Watershed (Fill–Spill–Merge, quadtree implicite, partage CPU/GPU, détection à 2 m). Commit `268f9da` sur la branche `patchterrain-et-plans`. Création de ce coffre. Pointeur vers le coffre ajouté au `CLAUDE.md`. Plans Traces GPU et Corde Niagara (API vérifiées dans le source 5.8). Nuit (auteur absent) : plugin `GPUTrace` (CPU + GPU Niagara, nœud Blueprint asynchrone, debug, agent Bug), squelette `DwamLighting` + `NormalTransfer.ush`, `Snake Toy` ; guides de montage. Tout compile ; rien de testé en éditeur ; rien de commité. |
| 2026-10-03 | Tests de l'auteur : GPUTrace (moteur CPU) validé en Blueprint ; maillage adaptatif OK ; snake OK mais clignote sur les murs (corrigé : temps de grâce) ; agent Bug corrigé (pas de 10 cm, marge, relance, errance, diagnostics). Comparatif transfert de normales : méthode de l'auteur (`MF_Light2`, gradient GDF à la surface + masque AO) préférée ; ajoutée à `NormalTransfer.ush` (variante C), plus une variante hybride D (deux sondes, base de mélange, sans AO, rejet du sol heuristique). Retard du GDF sur les objets mobiles (« reptation ») documenté avec les réglages à évaluer en fin de projet. |
| 2026-10-03 (soir) | Relecture complète du HLSL OKLCH de l'auteur (décodage UTF-16 des nœuds Custom). `OKLab.ush` : conversions, opérations en Lab (chroma, rotation de teinte, mélange), gamut mapping analytique d'Ottosson validé numériquement. `Lighting.ush` : garde-fou de normal map (bug des faces à l'ombre éclairées par le soleil), origine des rayons d'ombre, diffus OKLab (scotopique sur l'éclairement, L en énergie), 4 spéculaires. Guide `Guide_OKLab.md`. Pas testé en éditeur. |
| 2026-10-03 (nuit) | `Patterns.ush` (XOR et damier filtrés, Quilez). `Plan_PostProcess.md` : remplacement du tonemapper (vérifié dans le source), Hable + Tony McMapface, sRGB / gamma, réglages sauvegardés (SaveGame), éblouissement solaire façon Wind Waker (décompilation `d_kankyo_rain.cpp`) avec la version « dot + remap + visibilité » de l'auteur. **Plan en attente de validation**, `PostProcess.ush` pas écrit. Rien de cette soirée n'est committé. |
| 2026-10-03 (soir, suite) | L'auteur valide les trois plans (« tu peux faire tout ça »). **Post-process** : `PostProcess.ush` (exposition, Hable, Tony McMapface, sRGB / gamma, luminosité, éblouissement), module `DwamLightingRuntime` + composant `Dwam Sun Glare`, convertisseur `Tools/ConvertTonyMcMapfaceLut.ps1` (Unreal n'importe pas le format d'origine de la LUT), guide. **Eau** : plugin `Watershed` (hiérarchie de dépressions, Fill–Spill–Merge, tuiles en quadtree implicite, sources, pluie, debug), 5 tests automatiques réussis. **Corde** : contrainte Niagara trouvée (lecture de soi = frame précédente), donc plugin `Rope` en CPU (PBD, accroches, obstacles, décor, auto-collision, frottement, enroulement à bouts attachés, rendu en tube), 4 tests automatiques réussis. Tout compile ; rien n'est vu dans l'éditeur. |
| 2026-10-04 | Optimisations potentielles de Watershed notées dans [[Plan_Water]] (pluie par creux, tuiles déplacées au lieu d'être reconstruites, traces étalées, chronomètres). Pas faites : l'auteur teste d'abord en conditions réelles. |
| 2026-10-04 (suite) | Discussion loot, feu, persistance. Notes `Plan_Loot` (principes de l'auteur : pas de hiérarchie d'âge, la valeur n'est pas la qualité, âge d'or = signatures surreprésentées ; structure vérité / marquages / connaissance, objets en pièces), `Plan_Fire` (nodes construits autour du feu, idée de l'auteur), `Plan_WorldState` (graine + différences, monde qui guérit). |
| 2026-10-04 (3C) | Lecture des noms des assets du personnage de l'auteur (`Content/RPG/Characters/p1`). `Plan_3C` : vue subjective, main contrôlée (référence *For Honor*, pas d'active ragdoll), stances idle / interact / attack / defence / parade, Enhanced Input + routeur + sondes, outil visuel de filtrage (référence *EternalDahaka*), caméra par modes (Gameplay Cameras encore expérimental en 5.8). Idée EUW_Graph notée. |
| 2026-10-04 (combat) | `Plan_3C` enrichi : déplacements (glissade, coyote time + tampon de saut, escalade décor et monstres, enjamber avec le sprint, lever le pied dynamique, inertie en pente + outil, anti-coincement « code Konami »), combat (attaque par le mouvement, parade au poignet, rebond / glissement de l'arme), interactions (*Zack & Wiki*, bras de levier), gestes et formes (l'arme comme filtre, $1 Recognizer expliqué, tracé à la *Skyward Sword*). `Plan_Gauges` : avis sur le composant de jauges de l'auteur (pas à coder pour l'instant). |
| 2026-10-04 (State Tree) | `Guide_StateTree` : référence vérifiée dans le source 5.8 (types d'états, sélection dont l'utilité, transitions et priorités, catégories de liaison, événements par Gameplay Tag, schémas Component / AI, nœuds fournis, bases Blueprint et C++), bonnes pratiques, usages dans DWAM (IA, stances, mécanismes, Smart Objects, événements du feu, de l'eau et des jauges). |
| 2026-10-04 (Eiréné Nexus) | `Plan_EireneNexus` : idée de l'auteur (factions façon Nemesis system + théorie des jeux). Principes applicables (dilemme répété, faucon / colombe, chasse au cerf, dilemme de sécurité, coalitions, signaux coûteux, information incomplète…), esquisse, liens avec le loot (les factions produisent les périodes des marques), mise en garde sur le brevet du Nemesis system. |
| 2026-10-04 (potentiel vital) | `Plan_VitalPotential` : la vie comme monnaie cachée (tuer = absorber, partagé entre équipement et personnage), prototype de l'auteur repéré (`BP_EcoSystemVP_01b` : potentiel vital total, populations, grands nombres). `Plan_EireneNexus` : proximité = valeurs **et** géographie ; l'autre système est le potentiel vital. |
| 2026-10-04 (potentiel vital, suite) | Règles de l'auteur notées dans `Plan_VitalPotential` : équilibre conservé par acte (*Diablo 2*), somme nulle, factions qui dépensent comme dans un RTS, perte au combat, « la nature a horreur du vide », élites suivis, ponction entre actes, ambiances par l'équilibre, potions et autels, magie « boule de neige ». Lecture : modèle de stocks et de flux, la perte va à la nature, entiers 64 bits suffisants. |
| 2026-10-04 (mort imminente) | `Plan_VitalPotential` : on ne peut pas mourir, événements de mort imminente variés (sauvé, capturé, ramené, interrompu) qui restent à somme nulle (quelqu'un paie, dette envers une faction) ; ponction entre actes décidée par l'histoire, avec des outils ; exemple de la Comtesse de *Diablo 2* qui remplace Andariel comme boss. |
| 2026-10-04 (durabilité) | `Plan_Loot` : durabilité et bris façon *BOTW* (plus durables), pièces récupérables, équipement qui monte en niveau, énergie libérée et transformée au bris, gemmes au choix ; but : un build qui change, un build cassé qui ne dure pas, le bris vécu comme un gain (référence : l'XP reposée de *WoW*). |
| 2026-10-04 (bris) | `Plan_Loot` : modificateurs déclenchés au bris, éléments qui restent, hasard manipulable (craft en combat), priorité de l'énergie à l'objet le plus vide puis au joueur (remplissage par le bas, comme Watershed), réparation débloquée plus tard, gemmes trouvées ou nées du bris, pièce sauvée au hasard. Piste : l'énergie garde la « couleur » de sa source. |
| 2026-10-04 (équilibrage) | `Plan_Loot` : équilibre par un pierre-feuille-ciseaux **gigogne** (décision de l'auteur, référence *Destrega*), archétypes de jeu de combat à la base ; étages proposés (action, archétype, équipement) ; méthode (modificateurs-verbes, cibles d'expérience, simulation, jeux à étudier, version minuscule). Lien ajouté dans `Plan_3C`. |
| 2026-10-04 (PNJ) | `Plan_NPC` : fiches de personnage des PNJ (existant de l'auteur : `PDA_ProfileSheet_AgentLogic` / `AgentVisual`), fiche en modules, déformation du squelette indépendante du squelette (groupes d'os, longueur et épaisseur, IK des pieds), cheval compris ; Mutable en bêta en 5.8. |
| 2026-10-04 (PNJ, suite) | `Plan_NPC` : un seul squelette (le mannequin) pour bipèdes et quadrupèdes ; animaux montures et escaladables ; **pas de skinned mesh** : pièces rigides sur le squelette, jointures cachées par le transfert de normales (piste : la méthode du snake plutôt que le GDF, qui retarde sur les objets mobiles). |
| 2026-10-04 (tranche verticale) | `Plan_VerticalSlice` : brouillon (ce que la tranche doit prouver, contenu minuscule dans / hors tranche, six étapes jouables, critères de réussite, questions). Navigation Invokers notés dans `Plan_NPC`. |
| 2026-10-04 (scène) | `Plan_VerticalSlice` : la tranche doit prouver le monde, le côté RPG et la défaite sans mort (la main est déjà amusante, le loot est au cœur). Approche validée : un prototype de la boucle centré sur une scène (un chef exige la soumission ; le boss peut changer). |
| 2026-10-05 | Lecture du doc de l'auteur « Idées d'outils Unreal » (Claude Docs). Nouvelle section de l'accueil « Outils Unreal (pas strictement DWAM) » : `Plan_CityTool` (BP Anchor, édition façon Blender ; existant oublié `CitySplineTools`, sans lien avec `EUW_Graph`), `Plan_KitbashTool` (BP_Edition paramétrique), `Plan_ProceduralTrees`, `Plan_FromDustTerrain`. |
| 2026-10-05 (graphe) | `CitySplineTools` examiné : une surface triangulée à partir d'une spline, peu de route ; à part, voire à virer (auteur). `EUW_Graph` à garder. `Plan_Graph` : un objet graphe générique, remplaçant de la spline, socle commun à plusieurs projets. |
| 2026-10-05 (graphe v0) | `Plan_Graph` : plugin à part, réutilisable, strict minimum (décision de l'auteur). Version 0 proposée (`GraphKit` : acteur graphe, nœuds et arêtes avec GUID, tags, charge utile, poignées natives, API Blueprint, tests), en attente de validation. |
| 2026-10-05 (lieux rêvés) | `Plan_Graph` : socle sans position, des distances (décision de l'auteur), v0 = le socle seul. `Plan_DreamPlaces` : l'idée des chemins de l'auteur, des lieux rêvés reconnaissables malgré les variations (références : Kevin Lynch, Joris Dormans, *Unexplored*). `Plan_ProceduralTrees` : un graphe, plusieurs arbres. |
| 2026-10-05 (authoring) | Lieux rêvés écrits **à la main** (authoring indispensable ; Obsidian « horrible »), génération par la source ou le hasard plus tard. Références pour creuser ajoutées (Lynch, Dormans, *Unexplored*, Boris the Brave, PCG book, Hillier, Alexander). `Plan_Graph` : outil d'écriture des graphes (EUW ou éditeur d'asset, GenericGraph à étudier). `Plan_VitalPotential` : Machinations comme référence. |
| 2026-10-05 (principes) | Section « 🔥 Principes fondateurs » en tête de l'accueil (11 principes posés par l'auteur, avec leurs notes). `Plan_DreamPlaces` : plusieurs échelles d'un même lieu (plan du métro, carte détaillée, récit, jeu ; Harry Beck), lien avec *A-Life* (« loin du joueur, le monde vit sur le plan du métro ») ; « plus tard » corrigé : faire vivre les endroits une fois faits. |
| 2026-10-05 (bâtiments) | `Plan_KitbashTool` et `Plan_DreamPlaces` : bâtiments en kit + règles de mise en page + graphe ; l'échelle devient un réglage (Fort-Dragon compact, façon *Skyrim* ou façon *Witcher 3*), avec des éléments fixes à taille humaine et des éléments qui s'étirent. |
| 2026-10-05 (paramétrique) | `Plan_CityTool` : paramétrique emboîté (maison, quartier, ville), routes en graphe (carrefour = nœud, construit selon ses branches), simplification plutôt que jumeau numérique, pas un open world mais un monde en graphe façon *Diablo 2*. |
| 2026-10-05 (routes) | `Plan_CityTool` corrigé par l'auteur : les routes ne sont pas plus simples en graphe, c'est de la topologie (angles d'arrivée, G2, clothoïdes, bretelles, rues principales asymétriques) ; « ce n'est pas du cas limite, c'est LE cas », il faut du béton. Pistes : structure d'ASAM OpenDRIVE, hiérarchie des routes, catalogue de cas + tests automatiques. |
| 2026-10-05 (routes, suite) | `Plan_CityTool` : pas d'OpenDRIVE ni d'autoroutes (l'auteur le fait au travail) ; routes du jeu = chemins médiévaux, villes, treks, chemins de montagne ; le rond-point, intersection ou pas, est aussi un cul-de-sac. |
| 2026-10-05 (GraphKit + LayoutKit) | Plans écrits puis code avec l'accord de l'auteur. Plugin `GraphKit` (struct graphe sans position : nœuds nommés, arêtes avec distance, tags, charge utile ; bibliothèque Blueprint) et plugin `LayoutKit` (`ALayoutKitActor` : boîtes aux coins arrondis par coin, attaches sur les côtés, fixe \/ étirable, l'échelle de l'acteur lue comme une taille demandée). 8 tests automatiques réussis du premier coup. Guide `Guide_LayoutKit`. Pas encore vu dans l'éditeur. |
| 2026-10-05 (LOA) | Le curseur n'est pas une échelle mais le **LOA** (niveau d'abstraction), idée déjà amorcée par l'auteur (plugin `LOA` vide, « Topology Expander »). `Plan_LOA` : développement du graphe selon le LOA, ce qui ne disparaît jamais (l'identité), responsive design, disparition ou absorption, grammaire de graphes. Principe n° 3 complété. |
| 2026-10-05 (LOA v1) | Plugin `LOA` (celui de l'auteur, équipé d'un module) : élément LOA (visible jusqu'au niveau, s'efface ou absorbé), présence en cascade ; entier = structure, décimal = dépliage. `LayoutKit` : curseur LOA sur l'acteur, exemple avec terrasse (s'efface) et grange (absorbée). 11 tests réussis ; un test a révélé qu'une boîte entièrement absorbée doit rester (corrigé). |
| 2026-10-11 | Document de l'auteur « Clustered lighting par Z-binning » versé dans le coffre : `Plan_LightCulling` (Z-binning à la place de la grille de froxels envisagée jusque-là ; en-tête et liens ajoutés ; « 5.7 » corrigé en 5.8, coquille d'habitude de l'auteur ; points d'accroche vérifiés dans le source 5.8). Schéma « flux de données » retranscrit en Mermaid d'après une capture de l'auteur ; écart schéma / texte noté (1 thread par bin et par tuile, ou par lumière). Avis demandé : Z-binning préféré aux froxels a priori (tailles fixes, donc lisible en textures par les matériaux). Pas de code. |
| 2026-10-11 (ambiance) | `Plan_AmbientLights` : idée de l'auteur d'un type de lumière pour l'ambiance (plans émissifs / cubes, quadtree, placement GPU rêvé, pas de screen space ; exemple du citron dans la serviette). Budget des lumières directes : 32 à 64. Références notées (surfels de Bunnell et d'EA, cards de Lumen, splatting, VPL), proposition d'une v0 à la main. Comparatif des méthodes ; **l'auteur retient les surfels**. Six étapes proposées, en attente de validation ; il manque la disposition exacte du packing des lumières. Suite : données d'un émetteur (boîte orientée, 2 pixels : position, rotation, scale, couleur ; proposition), doute sur la faisabilité noté (à la main : oui ; « jeter les confettis » : risqué, couleur à l'impact). Idée de l'auteur : un authoring automatique en temps réel, qui ne couvre que les endroits à détailler et à déboucher, comme un éclairagiste. |
| 2026-10-11 (focus) | `Plan_Focus` : idée de l'auteur d'un système de focus (sphère d'attention mi-automatique, mi-authored, points d'intérêt triés par score, soleil compris, pour guider subtilement le regard ; cas de la sortie de caverne). Lecture : c'est le « directeur » des lumières d'ambiance en plus large ; les points d'intérêt rejoignent les repères de Lynch. Proposition en trois briques (point d'intérêt, subsystem, sorties MPC / Niagara / liste triée). Lecture confirmée par l'auteur « dans le principe ». Ajout de l'auteur : en PCG, la palette de couleurs du niveau indique quoi faire ressortir ; elle fournirait aussi la couleur des rebonds, sans la mesurer. |
| 2026-10-11 (lumières) | L'auteur écarte l'ambiance pour l'instant et demande la brique manquante des lumières. Existant relu : `BP_CustomLight` + `BP_LightManager` + `MPC_LightsShadow` (2 lumières, lues par `SHad_ShadowTest` seulement) ; **aucune trace dans le projet de la texture 1D à 2 pixels** citée ailleurs (question posée). `Plan_Lights` : propositions (lumières « fantômes » balayées, render target écrit sur le render thread, positions relatives à la caméra, packing 2 pixels, intensité maison, tri par importance), API vérifiées dans le source 5.8, six étapes, sept questions. En attente de validation ; pas de code. |
| 2026-10-11 (lumières, code) | Réponses de l'auteur : la texture n'existait pas (c'était sa piste) ; **pas de fantômes**, un composant maison (« j'aurai pas LEUR dette technique ») ; render target ok ; intensité 1 = pleine lumière ok sur le principe ; addition en RGB classique ; 8 layers suffisent ; soleil à part. Code dans `DwamLighting` : `UDwamLightComponent`, `ADwamLight`, `UDwamLightSubsystem`, `FDwamLightViewExtension`, réglages du projet, module `DwamLightingEditor` (gizmos), `Lights.ush`. Compile ; 3 tests automatiques réussis ; `Lights.ush` compilé hors moteur (DXC, FXC). `Plan_Lights` mis à jour, `Guide_Lights` écrit. `CLAUDE.md` : objectif des lumières aligné sur la décision. Rien de vu dans l'éditeur ; rien de commité. |
| 2026-10-11 (référence) | Nouvelle section « Références à creuser » dans l'accueil : vidéo *Surface-Stable Fractal Dithering Explained* (lien donné par l'auteur ; usage à préciser). Puis deux autres vidéos : mélange de normales (`BlendAngleCorrectedNormals`) et un exemple de rain drip shader ; puis la propagation d'ondes en Niagara de Ghislain Girardot, l'interception de cibles mobiles (Royal Skies) et un tutoriel de grinding en C++. |
| 2026-10-12 | Décision de l'auteur : **passer en forward shading dès que possible** (rendu « à l'ancienne », sans Lumen ni Nanite). `Plan_Forward` : effets vérifiés dans le source 5.8 (Nanite coupé, textures de scène du GBuffer interdites dans les matériaux, GDF conservé, SSR et sky light deferred coupés), impact par système (rien d'attendu sur l'éclairage maison, à contrôler dans l'éditeur), réglages proposés, étapes et retour arrière. Quatre questions posées ; aucun réglage modifié. |
