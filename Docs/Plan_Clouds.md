# Plan — Nuages « John Martin », ciel piloté

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Forward]] · [[Plan_PostProcess]] · [[Plan_Lights]] · [[Guide_Lights]] · [[Plan_LightCulling]] · [[Plan_KitbashTool]]

> Statut : 💭 **idée**. Le document d'origine (plus bas) a été écrit par une autre session Claude le 2026-10-12 et donné par l'auteur avec cet avertissement : « ils ont été faits un peu hors sol, donc attention ». La section « Confrontation au projet » est de la session qui l'a versé dans le coffre. Rien n'est décidé, rien n'est codé.

## Ce que l'auteur a dit (2026-10-12)

- « D'une manière ou d'une autre, il me faut des **nuages au sol**. »
- Un **ciel piloté** : oui.
- Ses shaders de nuages actuels fonctionnent **via le GDF** ; comme les nuages sont loin, **ils ne fonctionnent pas** (hors de portée du champ de distances).
- « Avec certaines choses que je veux mettre en scène, **ça ne scale pas**. » D'où : « il faut un système ».

- Sur le document : « c'est une **proposition**, hein, faite vite ». À prendre comme une piste, pas comme le plan.

- **Piste Nubis** (auteur, 2026-10-13) : « la méthode Nubis pour les nuages semble pas mal du tout, mais je ne sais pas à quel point je pourrais faire comme John Martin ». Réponse de Claude, de mémoire et sans relecture des présentations (à vérifier avant de s'en servir) : le Nubis d'origine (*Horizon Zero Dawn*, carte météo 2D + bruits 3D) donne des ciels naturels peu dirigeables et s'appuie sur la reprojection temporelle, que le projet s'interdit ; **Nubis³** (*Burning Shores*, nuages en voxels sculptés, champ de distances pour sauter le vide, détail ajouté au rendu, traversables) est la variante qui colle au besoin, et c'est en substance ce que propose le document ci-dessous. Le style Martin tient surtout à la composition, à l'éclairage dirigé et au contraste, plus qu'au modèle de densité.

- **Direction (auteur, 2026-10-17)** : « on va faire notre propre Nubis, a priori, en tout cas c'est un plan ». L'approche du doc « Plan de jeu UE5 » du 2026-10-16 (coquille maillée en smooth union + quelques pas de ray-march internes + bords effilochés) est écartée : « ses nuages sont nuls ».

## Lecture de « Nubis, Evolved » (2026-10-13)

Source donnée par l'auteur : les diapositives d'Andrew Schneider (Guerrilla), *Nubis, Evolved*, SIGGRAPH 2022 (*Advances in Real-Time Rendering in Games*), sur *Horizon Forbidden West*. **Limite de cette lecture** : seul le texte des 205 diapositives a pu être extrait (titres, formules, extraits de code) ; ni les images ni le commentaire oral. Ce n'est pas Nubis³ (2023) : la note « piste Nubis » ci-dessus, écrite de mémoire, **sous-estimait** ce que cette version permet de diriger.

### Ce que contient la présentation

| Partie | Contenu | Intérêt pour le projet |
|---|---|---|
| **Ciels** : modèle à profil vertical | Densité = bruit composite (Perlin-Worley, 4 canaux, 128³) − (1 − profil) ; profil = profil vertical × couverture. Pas de ray-march qui s'allonge avec la distance | Nuages lointains « naturels » |
| **Cirrus** : modèle 2,5D | Couche fine, sans ray-march volumique | Fond de ciel peu coûteux |
| **Authoring** | Des champs peints ou générés (« NDF ») dans Houdini ; des « Influence NDF » régionaux, mélangés selon l'état de la météo (calme, orageux, clair) | **Le « ciel piloté »** : des états de météo qui mélangent des cartes |
| **Environnements** : modèle « enveloppe » | Nuages accrochés au relief (orographiques), décrits par des cartes 2D : hauteur min, hauteur max, type, densité, **distance**. Référence visuelle citée : les **peintures d'Albert Bierstadt** | **Les nuages au sol**, avec des données 2D, donc légères |
| **Ray-march accéléré** | *Sphere tracing* et *cone step mapping* grâce à la distance stockée : un même plan passe de **4,2 ms à 1,3 ms** | Indispensable sans temporel |
| **VFX : la super-tempête** | Buts affichés : « réaliste, puissant, menaçant ». Mésocyclone et enclume sculptés par des cartes, rotation par anneaux (champ de vortex), et un éclairage **truqué à la main** : atténuation qui change avec la distance au centre, ambiance mélangée par zone, **lueur interne** ajoutée, éclairs | **Le plus proche de John Martin** : des masses monumentales, une lumière qui vient de l'intérieur |
| **Intégration** | Les nuages se fondent dans l'atmosphère selon une distance pondérée par l'opacité | Le fog et les nuages cohérents |
| **Volumes (NVDF)** | Aperçu de la suite : densité + distance dans un volume 3D | La piste du document ci-dessous, et de Nubis³ |

### Le modèle de lumière (à reprendre tel quel comme point de départ)

- Énergie = diffusion directe + diffusion ambiante.
- Directe = transmittance × phase primaire + diffusion multiple × phase secondaire (Henyey-Greenstein).
- Ambiante = `pow(1 − profil, 0,5)` : le « profil » (à quel point on est loin du bord du nuage) sert à la fois à l'ambiance et à la diffusion multiple. C'est le même levier que la distance au bord d'un SDF dans le document ci-dessous.
- Pour la tempête : `énergie = directe + ambiante + lueur`, la lueur étant un terme purement artistique.

### Ce qui coince avec le projet

- **Le temporel.** Le code de la tempête construit une position « de l'image précédente » avec une matrice de reprojection : le rendu de Nubis s'appuie sur la reprojection temporelle, que le projet s'interdit. Les chiffres annoncés (PS5 : 2 à 3 ms, 96 à 180 échantillons par rayon de vue, 10 par rayon de lumière ; PS4 : 4 ms au plus, en 960 × 540) sont obtenus **avec** cette aide. Sans elle, il faut plus d'échantillons ou un filtrage spatial.
- **Houdini** pour fabriquer les cartes. À remplacer par un outil dans l'éditeur, ou par des cartes peintes.

### Autres références données par l'auteur le même jour (pas vues : Claude ne peut pas regarder une vidéo)

- Ghislain Girardot, *A deep dive into Volumetric Clouds & Systemic Weather in UE* : https://www.youtube.com/watch?v=d6xu2BQK3Kg
- Daniel Elliot Jones, *Creating vast skyscapes using volumetric clouds*, NZGDC 23 : https://www.youtube.com/watch?v=FYs8mihcuYY

## Confrontation au projet

### Vérifié dans le source 5.8

| Point du document | Résultat |
|---|---|
| `PrePostProcessPass_RenderThread` | Existe (`SceneViewExtension.h:210`), reçoit `FPostProcessingInputs` |
| `SubscribeToPostProcessingPass` | Existe (`SceneViewExtension.h:222`) ; depuis 5.5 la signature prend la `FSceneView`. Les passes disponibles comprennent `ReplacingTonemapper`, `MotionBlur`, `Tonemap`, `FXAA` |
| `r.Fog`, `r.VolumetricCloud` | Existent (`FogRendering.cpp:25`, `VolumetricCloudRendering.cpp:48`) |

**Pas vérifié** : l'état (multi-échantillonné ou résolu) de la couleur et de la profondeur de scène à ce point d'accroche en forward avec MSAA ; `ERDGPassFlags::AsyncCompute` ; tous les budgets en millisecondes, qui sont des estimations.

### Ce qui ne colle pas avec le projet tel qu'il est

1. **« Forward+ maison »** : non. Le projet utilise le forward standard d'Unreal, activé le 2026-10-12, avec **MSAA 4x** ([[Plan_Forward]]). Le point « MSAA, à prévoir tôt » du document est donc immédiat, pas hypothétique.
2. **Tonemapper** : déjà traité par [[Plan_PostProcess]] (matériau de post-process en « Replacing the Tonemapper », vérifié dans le source). L'incertitude et le repli du document n'ont plus lieu d'être.
3. **Squelette du plugin** (question ouverte n° 3 du document) : une Scene View Extension existe déjà, `FDwamLightViewExtension`, dans `DwamLighting`.
4. **Nuages au sol** (question ouverte n° 1) : la caméra traverse les formations. Le « pire cas » du document (caméra dans une masse, plein écran) devient le cas courant. Ça pousse vers une structure commune (clipmap) plutôt qu'un volume par masse, et les budgets annoncés sont à refaire pour ce cas.
5. **Ombres des nuages au sol** : le document prévoit une shadow map de nuage, mais ne dit pas qui la lit. Or les matériaux du projet sont unlit et calculent leur éclairage eux-mêmes : il faut **exposer cette shadow map aux matériaux** (même transport que `RT_DwamLights`) et la multiplier dans l'ombre du soleil de `DwamShadeSurface` ([[Guide_Lights]]).
6. **Portée du GDF** : le document règle le problème de l'auteur sans le nommer, puisque les SDF des nuages sont cuits dans des volumes à part et ne dépendent plus du GDF du moteur.

### Rapprochements possibles (lecture de Claude, à discuter)

- **Lumières « célestes » artistiques** : plutôt qu'un nouveau type de lumière, des `Dwam Light` sur un layer réservé aux nuages (8 layers disponibles).
- **Ambiance** : le ciel rendu peut fournir l'`AmbientColor` de `DwamShadeSurface`.
- **Fog** : le document le fait en post-process à partir de la profondeur ; la même journée, il a aussi été envisagé dans les matériaux. Avec des nuages au sol, fog et nuages se confondent : l'idée du document de les mettre sur la même infrastructure (froxels) mérite d'être regardée en premier. Pas tranché.
- **Outil de sculpture** : primitives SDF et unions lissées, comme l'idée de SDF pour LayoutKit ([[Plan_KitbashTool]]). Un outillage commun est envisageable.

### Questions à l'auteur

1. Quelles sont les « choses à mettre en scène » qui ne passent pas à l'échelle aujourd'hui ?
2. Le ciel est piloté **par quoi** : l'heure, la météo, l'histoire (les actes), des volumes posés à la main ?
3. Combien de masses à l'écran, et à quelle échelle (colonnes de plusieurs kilomètres, bancs de brume à hauteur d'homme, les deux) ?
4. Machine et résolution visées, pour fixer un budget.
5. Le code va-t-il dans `DwamLighting`, ou dans un plugin à part ?

---

## Document d'origine (autre session Claude, 2026-10-12, non modifié)

## Système de nuages « John Martin » — UE 5.8, forward, sans Nanite

Notes de conception. Les points d'API sont basés sur l'architecture UE 5.4–5.6 : à vérifier dans le source 5.8.

### Contexte et contraintes

- Unreal Engine 5.8, **sans modification du moteur** (plugin uniquement)
- Pipeline **forward** (type Forward+ maison), pas de Nanite
- **Pas de filtres temporels** (pas de TAA/TSR, pas de reprojection)
- Remplacement du tonemapper et des fogs d'Unreal
- Temps réel, avec une exigence de scalabilité : les nuages ne doivent pas empêcher d'ajouter d'autres systèmes

### Ce qui fait un nuage « John Martin »

Références : *The Great Day of His Wrath*, *Belshazzar's Feast*, *The Deluge*.

- **Masses architecturales** : colonnes, falaises, murs de nuages monumentaux, composition dirigée
- **Source de lumière unique, quasi divine**, qui perce l'obscurité : cœur lumineux, bords éclairés (rim light), rayons
- **Contraste extrême** : noirs profonds contre or, blanc ou soufre
- **Plans étagés** avec forte perspective atmosphérique, pour l'échelle et la démesure

Conclusion : le Perlin-Worley classique piloté par une couverture nuageuse ne suffit pas. Il faut un pipeline **dirigeable par l'artiste**.

### Architecture générale

#### Représentation de la densité (deux couches)

1. **Macro-forme** : masses sculptées avec des primitives SDF (smooth union, soustraction, intersection, déformations), bakées dans un volume 3D
2. **Détail** : érosion Worley/curl 3D, évaluée au runtime uniquement sur les bords de la macro-forme

#### Rendu

- Raymarching en compute shader, résolution réduite (1/2 ou 1/4), upsample bilatéral guidé par la depth
- Éclairage :
  - phase function Henyey-Greenstein à double lobe (forte diffusion avant pour les liserés)
  - approximation de diffusion multiple (type Wrenninge) pour garder un cœur lumineux
  - **lumières artistiques non physiques** : sources « célestes » placées à la main, indépendantes du soleil (clé du style)
- **Shadow map de nuage** (profondeur optique vue de la lumière) pour les ombres au sol et les god rays, basse résolution, mise à jour rare

### Intégration dans le RDG (sans toucher au moteur)

Plugin avec `ISceneViewExtension` qui injecte les passes dans le Render Dependency Graph.

- **`PrePostProcessPass_RenderThread`** : accès à SceneColor et SceneDepth après la base pass. Point d'insertion des nuages et du brouillard.
- **`SubscribeToPostProcessingPass`** : accroche avant/après les passes du post-process.
  - Incertain : le callback remplace-t-il la passe du moteur ou s'exécute-t-il seulement autour ? Lire `PostProcessing.cpp`.
  - Repli : neutraliser le tonemapper du moteur (post-process volume neutre) et appliquer le tien en dernière passe.
- **Ordre conseillé** : nuages → brouillard (doit aussi s'appliquer aux nuages pour la perspective atmosphérique) → bloom → tonemapper maison
- Shaders exposés via `AddShaderSourceDirectoryMapping`
- Désactiver `VolumetricCloud` (`r.VolumetricCloud 0`)

#### Brouillard en forward

- Le fog du moteur est calculé dans la base pass des matériaux : on le **coupe** (`r.Fog 0` ou pas d'ExponentialHeightFog) et on fait le sien en post à partir de la depth
- **Piège** : les translucides n'écrivent pas de depth et ne voient pas ce fog post-process. Solutions : material function de fog dans les matériaux translucides, ou translucides uniquement près de la caméra.

#### MSAA

Le forward permet le MSAA, mais le composite des nuages doit gérer la depth multi-échantillonnée (résolution de la depth avant le raymarch, ou composite par sample sur les bords). À prévoir tôt.

### Raymarching sans temporel

Sans reprojection, le bruit ne se masque pas facilement. Leviers :

- **Sphere tracing** sur les SDF (saut du vide) plutôt que pas fixe
- Pas fixes uniquement dans l'enveloppe, jitter de **bruit bleu statique** par pixel + filtre spatial
- Demi-résolution + upsample bilatéral guidé par la depth
- **LOD du détail** : érosion évaluée seulement si densité de la macro-forme > 0 et seulement sur les premiers pas
- Early-out sur la transmittance (< 0.01)

### Les SDF : jusqu'où aller

**Ne pas évaluer les SDF en analytique dans le raymarch.** Au-delà d'une quinzaine de primitives en direct, c'est trop cher.

#### Solution : bake dans un volume 3D

- Primitives et arbre d'opérations côté éditeur
- Compute shader qui « cuit » le résultat dans une Volume Texture (SDF en R16F, 128³ à 256³ par masse), uniquement lors de la sculpture
- Runtime : uniquement des samples de texture, quel que soit le nombre d'opérations
- Gros ciels : un volume par masse (avec sa transformation) ou un clipmap autour de la caméra
- Animation lente : re-bake des régions modifiées seulement

#### Pièges

- **Une SDF modifiée n'est plus exacte** (smooth union, soustraction, displacement par bruit surestiment la distance). Sous-pas (distance × 0.5–0.8) ou clamp, ou redistancing en fin de bake.
- **Résolution** : une SDF baked lisse les détails fins, d'où l'érosion séparée au runtime
- **SDF ≠ densité** : `densité = smoothstep(0, épaisseur, -d) × érosion`. Le profil contrôle la douceur des bords.

#### Avantage pour le style Martin

La valeur `d` et son gradient donnent gratuitement la distance au bord et une normale : rim light, liserés lumineux, diffusion multiple approximée (plus on est loin du bord, plus on éclaire « de l'intérieur »).

#### Limite

Les SDF excellent pour les masses architecturales. Pour des formes très filamenteuses ou chaotiques, l'érosion doit faire plus de travail.

### Scalabilité : où sont les limites

Le coût dépend de **pixels × pas × samples**, pas de la complexité de la scène (si tout est baked).

#### Goulots, par ordre

1. Bande passante des samples de texture 3D dans le raymarch
2. Surface d'écran couverte : le pire cas est la caméra *dans* un nuage
3. Overdraw de volumes (plusieurs masses qui se chevauchent = plusieurs marches)
4. Mémoire et temps de bake : 256³ R16F ≈ 33 Mo, 128³ ≈ 4 Mo

#### Ce qui fait scaler

- **Une seule marche par pixel pour tout le ciel**, via une structure commune (clipmap ou grille éparse)
- **Culling par tuiles** 16×16 : une passe grossière teste les tuiles contre les enveloppes nuageuses
- Early-out sur transmittance et saut du vide via la distance baked
- **Bake étalé dans le temps** : budget en ms par frame, régions modifiées en file d'attente
- **Brouillard et nuages sur la même infrastructure** : brouillard volumétrique en froxels (basse résolution) pour la transmittance et l'inscattering
- **Async compute** (`ERDGPassFlags::AsyncCompute`) pour recouvrir la base pass, si la cible le supporte bien

#### Leviers de qualité (à exposer en cvars dès le départ)

| Levier | Effet |
|---|---|
| Échelle de résolution (1/2, 1/4) | coût divisé par 2 à 4 |
| Pas max | plafonne le pire cas |
| Érosion Worley on/off | retire une 2e lecture par pas |
| Nombre de samples de lumière | 1 à 3, voire 0 avec la shadow map |
| Résolution de la shadow map de nuage | marginal |

#### Budget visé (estimation à mesurer)

- 1.5 à 2.5 ms à 1080p sur GPU milieu de gamme
- Paliers de qualité descendant sous 1 ms pour les machines faibles
- Mesurer avec `stat gpu` / GPU Profiler **sur la vraie cible**, et toujours sur le **pire cas** (caméra dans une masse, plein écran)

### Feuille de route

1. Raymarcher basique + composition (nuage de test)
2. Macro-formes SDF bakées + érosion
3. Lumières artistiques + diffusion multiple
4. Shadow map de nuage et rayons
5. Optimisations spatiales (tuiles, demi-résolution, upsample bilatéral)
6. Intégration brouillard / SkyLight (cubemap rendue par toi pour alimenter le SkyLight) / tonemapper
7. Outils d'édition (acteur « CloudSculptor » : gizmos pour les masses, lumières célestes, preview éditeur)

### Questions ouvertes

- Nombre de masses nuageuses simultanées à l'écran, et échelle (nuages lointains uniquement, ou formations traversées par la caméra) : décide entre un volume par masse et un clipmap
- Machine minimale ciblée et résolution : pour affiner les budgets
- Où se branche le pipeline forward actuel (déjà dans une view extension RDG, ou base séparée) : pour définir le squelette du plugin
