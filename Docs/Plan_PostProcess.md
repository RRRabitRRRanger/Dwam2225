# Plan — Post-process maison : exposition, tonemapper, gamma, réglages

> Accueil : [[00 - Accueil]] · Lié à : [[Guide_OKLab]]

> Statut : **validé** par l'auteur (2026-10-03, « tu peux faire tout ça »). Étapes 1, 2 et 4 codées : `PostProcess.ush`, composant `Dwam Sun Glare`, convertisseur de LUT, guide [[Guide_PostProcess]]. Pas testé en éditeur.

## Objectif

Maîtriser la conversion finale de l'image vers l'écran, au lieu de la subir :

- exposition fixe, réglable ;
- courbe de tonemapping au choix : **Hable (Uncharted 2)** et **Tony McMapface** (périmètre volontairement réduit à ces deux-là pour l'instant) ;
- encodage final : **sRGB** ou **gamma** réglable ;
- réglages pilotés par un **menu de paramètres** du jeu (Blueprint, sans subsystem, comme prévu dans le coffre de l'auteur).

## Ce qu'Unreal fait aujourd'hui à une image « unlit »

Unlit ne veut pas dire intouché : le moteur ne calcule pas l'éclairage, mais applique ensuite :

| Effet | Où il vit | Décision |
|---|---|---|
| Brouillard | Acteur `ExponentialHeightFog` (une boîte de réglages ; le calcul est fait par le moteur, y compris dans les shaders des transparents et en volume) | Supprimé de la map (l'auteur a un fog maison) |
| Perspective aérienne | Acteur `SkyAtmosphere` (voile appliqué aux opaques et aux transparents, couleur du soleil, ciel) | **Supprimé.** Vérifié dans `SceneRendering.cpp` (5.8) : `SkyAtmosphereLightDirection` lit la lumière directionnelle marquée *Atmosphere Sun Light*, renseignée même sans SkyAtmosphere. Garder le soleil avec cette case cochée. L'auteur prévoit sa propre atmosphère (même principe : couleur du soleil selon l'angle, voile en distance) |
| Exposition automatique | Réglage projet *Auto Exposure* / Post Process Volume | Désactivée (déjà fait selon le coffre de l'auteur) ; exposition gérée par notre matériau |
| Tonemapper filmique | Toujours actif, sauf remplacement | **Remplacé** (voir plus bas) |
| Bloom, vignettage, grain… | Post Process Volume | Désactivés dans le template de map |

**Conséquence pour le pipeline maison** : comme Unreal, le fog et le voile atmosphérique maison devront être appliqués **dans les shaders des matériaux transparents** (un post-process ne voit qu'une profondeur par pixel). Prévoir des fonctions `.ush` partagées entre le post-process (opaques) et les matériaux transparents.

## Remplacement du tonemapper (vérifié dans le source 5.8)

Un matériau de post-process en *Blendable Location* = **Replacing the Tonemapper** :

- quand il est présent, la passe de tonemapping d'Unreal **n'est pas exécutée** (`PostProcessing.cpp`) ;
- il reçoit la couleur de scène **HDR linéaire** (`SceneTexture : PostProcessInput0`) ;
- sa sortie part vers l'écran : il doit donc faire **exposition → courbe → encodage** lui-même ;
- l'option matériau *Disable Pre-exposure Scale* contrôle la pré-exposition d'Unreal (à fixer au premier test).

`À VALIDER` au premier essai : l'encodage attendu en sortie (sRGB vs linéaire selon la cible d'affichage), avec une carte de gris : un gris moyen 18 % doit apparaître au milieu de l'écran (≈ 118/255 en sRGB).

## Architecture

| Élément | Qui | Rôle |
|---|---|---|
| `Plugins/DwamLighting/Shaders/PostProcess.ush` | Claude | Exposition, Hable, échantillonnage de la LUT de Tony McMapface, encodage sRGB / gamma. Pure maths, validé par compilation |
| `MPC_DwamPost` (Material Parameter Collection) | Auteur | Paramètres partagés : `Exposure`, `Tonemapper` (0 = aucun, 1 = Hable, 2 = Tony McMapface), `Gamma`, `UseSRGB` |
| `PP_DwamTonemap` (matériau de post-process, *Replacing the Tonemapper*) | Auteur, en suivant le guide | Un nœud Custom qui appelle la bibliothèque avec les paramètres de la MPC |
| Menu de réglages (UMG, Blueprint) | Auteur | Curseurs et listes qui écrivent dans la MPC (`Set Scalar Parameter Value`) ; sauvegarde des préférences |
| LUT de Tony McMapface | Auteur | À importer comme **Volume Texture** (voir plus bas) |

## Sauvegarde des réglages

Les réglages du menu doivent survivre à un redémarrage du jeu :

- un **SaveGame Blueprint** (ex. `SG_Settings`) qui contient une struct `S_DisplaySettings` (`Exposure`, `Tonemapper`, `Gamma`, `UseSRGB`, et les futurs réglages) ;
- au lancement, le jeu charge la sauvegarde (ou crée les valeurs par défaut) et **applique** chaque valeur à `MPC_DwamPost` ;
- le menu modifie la MPC en direct (aperçu immédiat) ; **Appliquer** sauvegarde, **Annuler** recharge la sauvegarde ;
- en Blueprint, sans subsystem (préférence de l'auteur). Alternative plus tard : une sous-classe C++ de `UGameUserSettings` (fichier `GameUserSettings.ini`, standard d'Unreal pour la résolution, la qualité, etc.).

**Anticrénelage (demande de l'auteur, 2026-10-12)** : le MSAA doit être réglable dans le futur menu. Depuis le passage en forward ([[Plan_Forward]]), le projet est en MSAA 4x. Proposition : une liste « Anticrénelage » (aucun, MSAA 2x, 4x, 8x) dans `S_DisplaySettings`, appliquée par les variables console `r.AntiAliasingMethod` (0 = aucun, 3 = MSAA) et `r.MSAACount` (2, 4, 8) avec le nœud `Execute Console Command`. Le source 5.8 décrit le MSAA comme réglable à l'exécution (`RenderUtils.cpp:1702`) ; **pas testé**.

## Les deux courbes

- **Hable (Uncharted 2)** : courbe filmique analytique (épaule, pied, point blanc). Rien à importer. Les paramètres d'origine sont repris ; seuls l'exposition et le point blanc sont exposés.
- **Tony McMapface** (Tomasz Stachowiak, tonemapper par défaut de Bevy) : préserve les teintes et évite les virages de couleur en haute lumière. Il repose sur une **LUT 3D** fournie par l'auteur (48³ texels). `À VÉRIFIER` : la licence du dépôt, et la procédure d'import en Volume Texture dans Unreal 5.8 (format du fichier fourni, conversion éventuelle en tranches 2D puis *Create Volume Texture*).

## Exposition : fixe + éblouissement solaire façon Wind Waker

Pas d'auto-exposition par histogramme (pompage, perte de lisibilité, réinterprétation de l'éclairage voulu). À la place :

- **Exposition fixe**, réglable dans le menu.
- **Éblouissement solaire** : l'effet de Wind Waker, vérifié dans la décompilation (`zeldaret/tww`, `src/d/d_kankyo_rain.cpp`) :
  - **Visibilité du soleil** : 5 lectures du Z-buffer autour de la position écran du soleil (centre + 4 points décalés de ~10 à 20 px) ; un point « au fond » voit le ciel. Donne `mVisibility` 0..1.
  - **Regard vers le soleil** : `falloff = 1 - min(distance au centre / 450 px, 1)`, puis `staring = falloff⁴`.
  - **Effet** : personnages et décor × `(1 - staring × visibility)` (jusqu'à la silhouette noire) ; brouillard et ciel × `(1 + staring × visibility × pulse × 0.5)` (`pulse` dépend de la hauteur du soleil).
- **Version DWAM** (hypothèse de l'auteur, plus propre) : le terme « regard » devient `dot(direction caméra, direction soleil)` → remap → courbe (puissance ou LUT 1D), indépendant de la résolution. La **visibilité** reste indispensable (sinon l'image s'assombrit derrière un mur) : lecture de `SceneDepth` en quelques points autour du soleil dans le matériau de post-process, ou un rayon GDF vers le soleil, lissée dans le temps. Le ciel se distingue des objets par la profondeur ; l'assombrissement peut porter surtout sur le L d'OKLab pour garder les teintes.
- Plus tard : **palettes d'environnement** par zone, comme les palettes de Wind Waker (`d_kankyo.cpp`), qui portent ambiance, exposition et brouillard, avec des réglages séparés personnages / décor pour la lisibilité et des fondus en OKLab entre zones.

## Template de map (checklist)

1. Pas d'`ExponentialHeightFog`.
2. Pas de `SkyAtmosphere` ; la lumière directionnelle du soleil garde *Atmosphere Sun Light* coché (source de `SkyAtmosphereLightDirection`).
3. Post Process Volume *Infinite Extent (Unbound)* : bloom, vignettage, grain, flou de mouvement à zéro ; exposition manuelle neutre.
4. `PP_DwamTonemap` dans ses *Post Process Materials*.
5. Matériau par défaut maison (motif XOR filtré, [[Guide_OKLab]]) sur les meshes de test.

## Étapes

1. `PostProcess.ush` (exposition, Hable, Tony McMapface par LUT, sRGB / gamma), validé par compilation.
2. Guide de montage : MPC, matériau de post-process, import de la LUT, menu de réglages de base (gamma, exposition, choix de la courbe), sauvegarde des réglages (SaveGame).
3. Premier test à la carte de gris, puis réglage de la pré-exposition et de l'encodage.
4. Éblouissement solaire (dot + remap + visibilité par profondeur), lissé dans le temps.
5. Plus tard : palettes d'environnement par zone ; autres réglages du menu.

## Idée ajoutée : impact frames (doc de l'auteur, 2026-10-16)

💭 Pas validé, hors du plan ci-dessus. **2 à 3 frames stylisées à l'impact d'un coup** ; le timing se teste. Précision de l'auteur : le *sakuga* est du contenu d'animation, pas un système. Lié à [[Plan_3C]] (combat).

## Décisions de mise en œuvre (2026-10-03)

- **Visibilité du soleil par 5 traces CPU** (collision, canal Visibility, complexe) plutôt que par lecture de `SceneDepth` dans le matériau : le lissage dans le temps demande un état d'une frame à l'autre, qu'un matériau de post-process n'a pas. Le composant calcule regard × visibilité, lisse, et écrit `SunGlare` dans la MPC. Limite : un objet sans collision ne cache pas le soleil.
- **Éblouissement appliqué en linéaire, avant la courbe** : multiplier la couleur par un scalaire garde la teinte (en OKLab, L, a et b changent ensemble).
- **Réglage `Brightness`** ajouté (gamma utilisateur, 1 = neutre) : c'est le curseur « luminosité » habituel des menus ; `Gamma` reste le gamma de l'écran en mode gamma pur.
- **LUT de Tony McMapface** : Unreal 5.8 n'importe pas le format du `.dds` d'origine, mais importe un DDS volume RGBA16F directement en Volume Texture (vérifié dans `DDSFile.cpp` et `EditorFactories.cpp`). D'où le script `Tools/ConvertTonyMcMapfaceLut.ps1`. Licence vérifiée : MIT ou Apache 2.0.
- Module `DwamLightingRuntime` (phase Default) ajouté au plugin pour les composants ; `DwamLighting` reste en PostConfigInit pour les shaders.
