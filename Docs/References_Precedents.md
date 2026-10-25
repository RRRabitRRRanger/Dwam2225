---
type: references
projet: DWAM
mis_a_jour: 2026-10-25
---

# Références et précédents de l'industrie

Retour à [[00 - Accueil]].

Les références que l'auteur donne comme **précédents** : des choses qui existent pour de vrai dans des moteurs, des jeux ou des publications. Elles servent de base avant de concevoir quoi que ce soit (« état de l'art d'abord »). Un plan de système qui touche un de ces sujets doit citer sa référence ici.

**Lecture de chaque fiche** :
- **Vérifié** : Claude a ouvert la source dans cette session.
- **De mémoire** : ce que Claude sait de la référence sans l'avoir rouverte ; à confirmer avant de s'en servir dans un plan.
- `À COMPLÉTER` : inconnu, à demander à l'auteur ou à lire.

L'auteur voudrait idéalement prendre une heure par référence pour en parler. Ce document est le point de départ de ces discussions, pas un résumé qui les remplace.

> **Dump 1/2 du 2026-10-25.** Liens donnés en vrac (préparés avec une autre IA), plus deux captures d'onglets. La partie 2 est annoncée.

---

## 1. Rendu, matériaux, BRDF

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| *The Devil is in the Details: idTech 666* (Doom 2016), SIGGRAPH 2016, Advances in Real-Time Rendering | https://advances.realtimerendering.com/s2016/Siggraph2016_idTech6.pdf | De mémoire : rendu **forward clustered** de Doom 2016 (lumières, decals et probes dans une grille de clusters), matériaux, anticrénelage temporel, budget à 60 i/s. | Précédent direct de [[Plan_Forward]] et [[Plan_LightCulling]] : un jeu AAA en forward avec culling de lumières en grille, ce que DWAM reproduit à la main. |
| *Physically Based Shading in Theory and Practice*, cours SIGGRAPH 2013 (Self Shadow) | https://blog.selfshadow.com/publications/s2013-shading-course/#course_content | De mémoire : la série de cours PBR de référence (cette année-là : Karis pour UE4, Neubelt et Pettineo pour *The Order: 1886*, etc.). | Base théorique des spéculaires de `Lighting.ush` ([[Guide_OKLab]]). |
| *Crafting a Next-Gen Material Pipeline for The Order: 1886*, Neubelt et Pettineo (SIGGRAPH 2013) | https://www.semanticscholar.org/paper/Crafting-a-Next-Gen-Material-Pipeline-for-The-Order-Neubelt-Pettineo/5e89fa62e24100a07301e6a335299f3b48f65f3e | De mémoire : pipeline de matériaux en couches composées hors ligne, BRDF spécialisées dont un modèle de tissu, anticrénelage spéculaire. | Rendu « photoréaliste à l'ancienne » : *The Order* est la référence visuelle la plus proche de l'intention de DWAM (forward, MSAA, matériaux très travaillés). |
| *The Order: 1886*, GDC 2014, David Neubelt (archive.org) | https://archive.org/details/GDC2014Neubelt/page/n47/mode/2up | Les diapositives GDC 2014 de la même équipe (page 47 pointée par l'auteur). Contenu de la page : `À COMPLÉTER`. | Idem. |
| **Ashikhmin** (nom seul) | — | Nom donné sans lien. De mémoire : Michael Ashikhmin, auteur du BRDF anisotrope Ashikhmin–Shirley (2000) et d'un modèle de **velours / tissu** par distribution de microfacettes (2007), celui que *The Order: 1886* a repris pour ses tissus. Lequel l'auteur visait : `À COMPLÉTER`. | Tissus des PNJ et des vêtements ([[Plan_NPC]], idée « Milka » du [[Catalogue_Features]]). |
| Sébastien Lagarde, blog et plan du site | https://seblagarde.wordpress.com/ · https://seblagarde.wordpress.com/site-map/ | De mémoire : ancien de DONTNOD et d'Unity, auteur de *Moving Frostbite to PBR*. Le plan du site recense ses articles. | Mine pour l'éclairage, la pluie (section 4) et la colorimétrie. |
| Bart Wronski, blog | https://bartwronski.com/ | Référence récurrente que l'auteur veut garder en tête (« une parmi d'autres »). De mémoire : traitement du signal appliqué au rendu, filtres, échantillonnage, couleur. | Lecture de fond ; à citer au cas par cas. |
| *MyLibrary*, Alexander Veselov (GitHub) | https://github.com/AlexanderVeselov/MyLibrary/blob/master/README.md | **Vérifié** : bibliographie commentée de l'infographie, de Phong (1975) à Instant NGP (2022) : GI, path tracing, ombres, BRDF, débruitage, Monte-Carlo, techniques de moteurs. Articles SIGGRAPH, EG, JCGT, HPG, GDC. | Index où chercher avant d'écrire un plan de rendu. |

## 2. Textures procédurales, champs d'orientation, répétition

L'auteur avait monté un **Voronoï nourri par des normales** (normale par pixel ou par vertex, comme une texture de flèches) au lieu de bruit, et obtenu une sorte de flow field. En en parlant, il a appris que c'était, en gros, une redécouverte du **Gabor flow** / des textures orientées. Voisin dans le coffre : l'orientation par cellule Voronoï des matériaux du terrain ([[Plan_PatchTerrain]]), reprise par l'eau pour le sens du courant ([[Plan_Water]]).

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| Nœud *Gabor* procédural de Blender (fil devtalk) | https://devtalk.blender.org/t/gabor-procedural-texture-node/25934/11 | De mémoire : discussion et implémentation d'un bruit de Gabor orientable dans Blender (arrivé dans Blender 4.3). Onglets ouverts : « Gabor flow », « Gabor Noise & Phasor Noise ». | Version « propre » de la méthode Voronoï + normales de l'auteur. |
| *Procedural Phasor Noise*, Tricard et al., SIGGRAPH 2019 | (onglets de l'auteur, pas de lien donné) | De mémoire : variante du bruit de Gabor qui sépare amplitude et phase : des motifs oscillants nets et orientables (rayures, ondes) sans perte de contraste. | Motifs orientés le long d'un champ (veinage, stries, vagues) ; voisin des motifs filtrés de `Patterns.ush`. |
| *Lapped Textures*, Praun, Finkelstein, Hoppe, SIGGRAPH 2000 | https://gfx.cs.princeton.edu/proj/lapped_tex/ · vidéos https://www.youtube.com/watch?v=Ix3QSAd17wk et https://www.youtube.com/watch?v=zT8akWQPoOU | De mémoire : on colle de petits patchs d'une texture exemple sur la surface, orientés par un champ de directions, qui se recouvrent. Les vidéos semblent être la présentation SIGGRAPH (titre d'onglet « Lapped textures (SIGGRAPH… ») : `À COMPLÉTER`. | Textures orientées sur les pièces du mannequin et les créatures ([[Plan_Mannequin]]), sans UV soignées. |
| *Layered Solid Texture Synthesis from a Single 2D Exemplar*, Takayama et Igarashi (poster, ~2009) | https://kenshi84.github.io/layered-solid/layered-solid-poster.pdf | **Vérifié** : synthèse d'une **texture volumique en couches** (strates de terrain, gâteau roulé) à partir d'une seule image 2D, sans les hachures de la méthode naïve. Suite des *Lapped Solid Textures* (SIGGRAPH 2008), qui remplissent l'intérieur d'un modèle. | **L'intérieur des objets** : ce qu'on voit quand on coupe (section 3) ou quand le terrain s'effondre (strates). |
| *Practical Real-Time Hex-Tiling*, Morten S. Mikkelsen, JCGT 2022 | https://jcgt.org/published/0011/03/05/ · démo https://github.com/mmikk/hextile-demo | De mémoire : casser la répétition d'une texture en la mélangeant sur une grille hexagonale avec rotations aléatoires, mélange qui préserve le contraste ; démo fournie. (« Pue la classe », dixit l'auteur.) | Sols et parois de [[Plan_PatchTerrain]] sans motif répété visible ; s'intègre dans un nœud Custom. |

## 3. Destruction et découpe

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| *Engineering Mayhem: Technical deep-dive into Environmental Destruction in THE FINALS*, Måns Isaksson (Embark), GDC 2024 | https://media.gdcvault.com/gdc2024/Slides/GDC+slide+presentations/Isaksson_Mans_Engineering+Mayham+Technical.pdf | **Vérifié** (48 diapositives). Quatre blocs : **authoring** (fracture dans Houdini, outil de bâtiments, connexions entre fragments générées puis visualisées, pièces posées sur un sol porteur), **simulation** (solveur maison de corps rigides, **analyse structurelle** : la force de chaque connexion décomposée en traction / compression, cisaillement et flexion, pondérées par matériau ; rupture quand l'impulsion dépasse une ligne de base + marge), **performance** (solveur direct creux par **Cholesky incrémentale** plutôt qu'impulsions séquentielles, qui répartissaient mal les efforts ; réseau par quantification + compression delta, ~400 → ~175 kbit/s ; *GPU transform pool* pour les milliers de pièces), **le reste** (lumière, edge meshes, VFX, déplacement). Précédents cités : *Battlefield: Bad Company*, *Red Faction: Guerrilla*, Glenn Fiedler (*Networked Physics*). | Le bris comme récompense ([[Plan_Loot]]), le feu et l'effondrement ([[Plan_Fire]]). **Le graphe de connexions entre fragments** est exactement un usage de [[Plan_Graph]] (nœuds = fragments, arêtes = liaisons avec charge). |
| *The Finals* : destruction progressive, mise à jour du 2026-10-05 (IDC Games) | https://idcgames.com/en/the-finals/news/the-finals-makes-buildings-bend,-swing-and-fight-back-in-its-biggest-destruction-upgrade-yet-2026-10-05-11-30-14024 | **Vérifié** : un état intermédiaire entre intact et gravats (murs et planchers qui penchent, s'affaissent, se tordent en restant attachés), éléments suspendus qui oscillent avant de tomber, ondes de choc qui déforment, gravats qui ouvrent des passages, fumée bloquée par les murs ; simulé côté serveur, lisibilité travaillée. | Même usage ; l'état « tordu mais tenu » est très proche d'une corde ou d'un PBD ([[Plan_NiagaraRope]]). |
| *Embark Studios: AI let devs do more with less when making The Finals* (Game Developer) | https://www.gamedeveloper.com/audio/embark-studios-ai-let-devs-do-more-with-less-when-making-the-finals | De mémoire : voix des commentateurs générées par synthèse vocale, outils d'IA pour une petite équipe. Contenu exact : `À COMPLÉTER`. | Voisin de [[Plan_KlattVoice]] (voix synthétiques en jeu). |
| *Metal Gear Rising* : la découpe (Game Art Tricks, Simon Schreibt) | https://simonschreibt.de/gat/metal-gear-rising-slicing/ | De mémoire : décortique comment le jeu coupe un mesh selon un plan en temps réel (deux moitiés, bouchage de la section, matériau d'intérieur). | Découpe à l'arme dans le combat de [[Plan_3C]] ; l'intérieur peut venir des textures volumiques (section 2). |
| Fil Reddit r/gamedev : découpe dynamique façon *Metal Gear* | https://www.reddit.com/r/gamedev/comments/49vqt5/game_mechanic_dynamic_mesh_cutting_metal_gear/ | De mémoire : discussion d'implémentation (plan de coupe, triangulation de la section, coût). | Idem. |

## 4. Pluie, eau, effets de surface

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| *Observe rainy world*, Sébastien Lagarde (2012) | https://seblagarde.wordpress.com/2012/12/10/observe-rainy-world/ | De mémoire : observation photographique de la pluie (surfaces mouillées plus sombres et plus brillantes, flaques, gouttes, éclaboussures, ruissellement) avant toute technique. | Météo et sols mouillés ; voisin de [[Plan_Water]] et [[Plan_Clouds]] (états de météo). La démarche (observer d'abord) rejoint la règle « état de l'art d'abord ». |
| *Water drop 2a – Dynamic rain and its effects*, Sébastien Lagarde (2012) | https://seblagarde.wordpress.com/2012/12/27/water-drop-2a-dynamic-rain-and-its-effects/ | De mémoire : la mise en œuvre temps réel (*Remember Me*) : pluie, éclaboussures, ruissellement sur les murs, gouttes sur la caméra. | Idem ; la vidéo de rain drip shader déjà notée dans l'accueil est voisine. |
| Deux vidéos YouTube | https://www.youtube.com/watch?v=23mQ93MrDeg · https://www.youtube.com/watch?v=n9LMmUS11aw | Titres non vérifiés. D'après les onglets de l'auteur, probablement les vidéos *Water drop 2a – Dynamic rain…* de Lagarde : `À COMPLÉTER`. | Idem. |

## 5. Terrain : érosion, coulées

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| *Efficient Debris-flow Simulation for Steep Terrain Erosion*, SIGGRAPH 2024 (HAL) | https://hal.science/hal-04574826v1/file/2024_Siggraph_Debris_Flow_Author_Version.pdf | Titre d'après l'onglet de l'auteur ; la page HAL bloque les robots (pas lue). De mémoire : équipe Galin / Peytavie : coulées de débris qui creusent les pentes raides (ravines), complément de l'érosion hydraulique. | Relief crédible pour [[Plan_PatchTerrain]] ; voisin de [[Plan_FromDustTerrain]]. |
| Article *Computer Graphics Forum* (Wiley, cgf.14992) | https://onlinelibrary.wiley.com/doi/full/10.1111/cgf.14992 | Page refusée (403). Titre : `À COMPLÉTER`. Peut-être lié à l'onglet « Patch Erosion for Deformable… » (vidéo), à vérifier. | Si c'est de l'érosion par patchs : très proche de [[Plan_PatchTerrain]]. |
| Fils Reddit sur l'érosion hydraulique | https://www.reddit.com/r/Unity3D/comments/rbnkcu/hydraulic_erosion_makes_terrain_look_so_good/ · https://www.reddit.com/r/proceduralgeneration/comments/16j6eme/improved_hydraulic_erosion_on_my_procedural/ | De mémoire : érosion par particules de gouttes (dans la lignée de Sebastian Lague), avant / après. | Version simple, à essayer avant les articles. |
| World Machine, blog (2012) | https://www.world-machine.com/blog/2012/06/269/ | Contenu exact `À COMPLÉTER` (mise à jour de juin 2012, d'après l'onglet). | Outil de référence pour le terrain érodé. |

## 6. Simulation de matière et sculpture

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| *GPU-Based Clay Simulation and Ray-Tracing Tech in Claybook*, Sebastian Aaltonen, GDC 2018 | https://media.gdcvault.com/gdc2018/presentations/Aaltonen_Sebastian_GPU_Based_Clay.pdf · vidéo (Digital Dragons 2018, d'après l'onglet) https://www.youtube.com/watch?v=Xpf7Ua3UqOA | De mémoire : monde entier en **SDF volumique** modifié en temps réel sur GPU, rendu par ray-tracing de SDF, physique des fluides et de l'argile. | Le projet vit déjà de champs de distance (ombres, jointures MDF / GDF, [[Plan_Mannequin]], `SDFEyes`) : c'est la référence d'un moteur qui les assume jusqu'au bout. Voisin de la question SDF de [[Plan_KitbashTool]]. |

## 7. Visages et yeux

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| Caustiques synthétiques dans l'iris, façon Jorge Jimenez | (onglet « Jimenez, GDC 2013 – Next-Gen… ») | De mémoire : *Next-Generation Character Rendering* (Jimenez, GDC 2013) : peau, et pour les yeux une **caustique simulée** sur l'iris (la cornée concentre la lumière du côté opposé à la source). | Œil de [[Plan_Eyes]]. Note : l'auteur s'intéresse d'abord aux paupières ; ici c'est un détail du globe qu'il demande lui-même. |
| *Porting Shadertoy to Unreal Engine*, viclw17 (2022) | https://viclw17.github.io/2022/02/08/port-shadertoy-to-unreal-engine | De mémoire : comment passer un shader Shadertoy dans un nœud Custom d'Unreal (coordonnées, `iTime`, fonctions). | Méthode déjà pratiquée dans le projet (nœuds Custom, `.ush`) ; utile pour porter des démos. |

## 8. Production

| Référence | Lien | Ce que c'est | Pour DWAM |
|---|---|---|---|
| *The key to a great video game performance? Team trust.*, Carli Velocci (Game Developer) | https://www.gamedeveloper.com/audio/the-key-to-a-great-video-game-performance-team-trust- | **Vérifié** : panel du GDC Festival of Gaming 2026 sur la confiance entre acteurs, directeurs de performance et équipes. Il s'agit de **performance d'acteur**, pas de performance technique. | Dialogues et visages ([[Plan_Face]]) si des voix enregistrées s'ajoutent un jour. Pourquoi l'auteur l'a mise ici : `À COMPLÉTER`. |

---

## Plans qui manquent (proposition de Claude, à valider)

L'auteur constate qu'il manque des plans sur des sujets qui ont des précédents solides. D'après ce dump, les trous les plus nets :

| Plan possible | Références | Ce qui existe déjà |
|---|---|---|
| `Plan_Materials` : modèle de matériaux, BRDF (tissu, anisotrope), couches | idTech 6, cours PBR 2013, *The Order*, Ashikhmin, Lagarde | `Lighting.ush` (4 spéculaires), mais aucun plan pour les matériaux eux-mêmes |
| `Plan_Destruction` : graphe de connexions, analyse structurelle, débris | *The Finals* (GDC 2024 + mise à jour 2026), *Red Faction*, *Bad Company* | Le bris des objets dans [[Plan_Loot]], [[Plan_Graph]] ; rien sur le décor |
| `Plan_Slicing` : découpe au plan, intérieur des objets | *Metal Gear Rising*, textures volumiques en couches | Rien |
| `Plan_Weather` : pluie, surfaces mouillées | Lagarde (*Remember Me*) | Nuages dans [[Plan_Clouds]], eau statique dans [[Plan_Water]] |
| `Plan_TextureSynthesis` : orientation, anti-répétition | Gabor / phasor, lapped textures, hex-tiling | Orientation Voronoï dans [[Plan_PatchTerrain]], `Patterns.ush` |
| `Plan_Erosion` (ou chapitre de [[Plan_PatchTerrain]]) | Debris flow 2024, érosion hydraulique, World Machine | Rien |

Rien n'est créé tant que l'auteur n'a pas choisi, et la partie 2 du dump peut changer cette liste.
