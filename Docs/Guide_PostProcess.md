# Guide — Post-process maison : montage dans l'éditeur

> Accueil : [[00 - Accueil]] · Plan : [[Plan_PostProcess]] · Lié à : [[Guide_OKLab]]

> État (2026-10-03) : `PostProcess.ush` validé par compilation (DXC SM6, FXC SM5, hors moteur). Composant `Dwam Sun Glare` compilé. Convertisseur de LUT testé sur une LUT synthétique (valeurs exactes). **Rien n'est testé dans l'éditeur.**

## Ce qui est fourni

| Fichier | Rôle |
|---|---|
| `Plugins/DwamLighting/Shaders/PostProcess.ush` (`/Plugin/DwamLighting/PostProcess.ush`) | Exposition, Hable, Tony McMapface, encodage sRGB / gamma, luminosité, éblouissement solaire |
| `Plugins/DwamLighting/Tools/ConvertTonyMcMapfaceLut.ps1` | Convertit la LUT `.dds` de Tony McMapface en DDS volume RGBA16F, qu'Unreal importe directement |
| Composant **Dwam Sun Glare** (module `DwamLightingRuntime`) | Calcule l'éblouissement (regard × visibilité, lissé) et l'écrit dans la MPC |

## 1. Préparer la map

Voir la checklist « Template de map » de [[Plan_PostProcess]]. En résumé :

- pas d'`ExponentialHeightFog`, pas de `SkyAtmosphere` ;
- le soleil (Directional Light) garde **Atmosphere Sun Light** coché ;
- un Post Process Volume **Infinite Extent (Unbound)** : bloom, vignettage, grain, flou de mouvement à 0 ; *Exposure* : **Metering Mode = Manual**, **Exposure Compensation = 0**, **Apply Physical Camera Exposure = décoché** (l'exposition d'Unreal vaut alors 1 : c'est notre matériau qui expose).

## 2. La LUT de Tony McMapface

1. Télécharger `shader/tony_mc_mapface.dds` depuis https://github.com/h3r2tic/tony-mc-mapface (licence MIT ou Apache 2.0, au choix : usage commercial permis, garder la mention de licence dans les crédits).
2. La convertir (PowerShell) :
   ```
   & "D:\Nos Documents\Doc Alexandre\Unreal Projects\Dwam2225 5.8\Plugins\DwamLighting\Tools\ConvertTonyMcMapfaceLut.ps1" -InputPath "C:\Téléchargements\tony_mc_mapface.dds" -OutputPath "C:\Téléchargements\T_TonyMcMapface.dds"
   ```
   Le script affiche les dimensions (attendu : `48x48x48`) et la valeur max.
3. Glisser `T_TonyMcMapface.dds` dans le Content Browser (ex. `Content/AVOLABO/PostProcess/`). Unreal 5.8 le reconnaît comme **Volume Texture**.
4. Ouvrir la texture et vérifier : *Compression* = **HDR** (mis automatiquement pour un format flottant), *sRGB* décoché, *Mip Gen Settings* = **NoMipmaps**, *Tiling Method* = **Clamp** (dans *Advanced*).

## 3. La collection `MPC_DwamPost`

Material Parameter Collection, paramètres scalaires :

| Paramètre | Défaut | Rôle |
|---|---|---|
| `Exposure` | 0 | Exposition en EV (+1 = deux fois plus clair) |
| `Tonemapper` | 1 | 0 = aucune courbe, 1 = Hable, 2 = Tony McMapface |
| `WhitePoint` | 11.2 | Hable : valeur linéaire qui devient le blanc |
| `UseSRGB` | 1 | 1 = courbe sRGB exacte, 0 = gamma pur |
| `Gamma` | 2.2 | Gamma pur (si `UseSRGB` = 0). **1 = aucun encodage** (voir test à la carte de gris) |
| `Brightness` | 1 | Réglage « luminosité » du menu : > 1 éclaircit les tons moyens |
| `SunGlare` | 0 | Écrit par le composant Dwam Sun Glare, ne pas toucher |
| `GlareDarken` | 0.6 | Assombrissement max des objets (1 = silhouette noire, comme Wind Waker) |
| `GlareSkyBoost` | 0.5 | Éclaircissement max du ciel |
| `SkyDistance` | 1000000 | Au-delà (cm), c'est le ciel. À régler selon la sphère de ciel (≈ 0,9 × son rayon) |
| `SkyFade` | 10000 | Largeur du fondu objet → ciel (cm) |

## 4. Le matériau `PP_DwamTonemap`

1. Nouveau matériau. *Material Domain* = **Post Process**, *Blendable Location* = **Replacing the Tonemapper**.
2. Nœud **Custom** : *Output Type* = **CMOT Float 3**, *Include File Paths* : `/Plugin/DwamLighting/PostProcess.ush`.
3. Entrées du Custom (noms exacts) :

| Entrée | Branchement |
|---|---|
| `SceneColor` | **SceneTexture : PostProcessInput0**, sortie *Color*, masque RGB |
| `Depth` | **SceneTexture : SceneDepth**, sortie *Color*, masque R |
| `Lut` | **Texture Object** → `T_TonyMcMapface` |
| `Exposure`, `Tonemapper`, `WhitePoint`, `UseSRGB`, `Gamma`, `Brightness`, `SunGlare`, `GlareDarken`, `GlareSkyBoost`, `SkyDistance`, `SkyFade` | Nœuds **Collection Parameter** de `MPC_DwamPost` |

4. Code :
   ```hlsl
   float Sky = DwamSkyMask(Depth, SkyDistance, SkyFade);
   float3 C = DwamApplySunGlare(SceneColor, SunGlare, Sky, GlareDarken, GlareSkyBoost);
   return DwamTonemap(C, Exposure, Tonemapper, WhitePoint, Lut, LutSampler, UseSRGB, Gamma, Brightness);
   ```
   (`LutSampler` est créé automatiquement par Unreal pour l'entrée `Lut`.)
5. Sortie du Custom → **Emissive Color**.
6. Dans le Post Process Volume : *Rendering Features → Post Process Materials* → ajouter `PP_DwamTonemap`.

## 5. Premier test : la carte de gris

But : vérifier qu'il n'y a ni double encodage ni encodage manquant.

1. Un plan face caméra, matériau **Unlit**, *Emissive* = 0.18 (gris moyen 18 %).
2. `Tonemapper` = 0, `Exposure` = 0, `UseSRGB` = 1, `Brightness` = 1.
3. Capture d'écran, pipette sur le plan :
   - **≈ 118 / 255** : correct ;
   - **≈ 187 / 255** (image délavée) : Unreal encode une seconde fois. Mettre `UseSRGB` = 0 et `Gamma` = 1 (aucun encodage), puis le noter dans [[Plan_PostProcess]] ;
   - **≈ 46 / 255** (image très sombre) : l'encodage manque. Le noter : il faudra chercher où.
4. Si l'image paraît décalée d'un facteur constant (toutes les valeurs trop fortes ou trop faibles) : c'est la pré-exposition. Essayer l'option du matériau **Disable Pre-exposure Scale**.

Ensuite, comparer `Tonemapper` 1 et 2 sur une scène réelle. Tony McMapface demande souvent une `Exposure` un peu plus haute que Hable.

## 6. L'éblouissement solaire

1. Sur le Blueprint du Player Controller (ou du Pawn) : *Add Component* → **Dwam Sun Glare**.
2. *Parameter Collection* = `MPC_DwamPost` ; *Parameter Name* = `SunGlare`.
3. *Sun Actor* : vide (il prend le soleil marqué *Atmosphere Sun Light*), ou l'acteur du soleil.
4. Jouer, cocher **Debug** : traces jaunes (libres) et rouges (bloquées) vers le soleil, valeurs à l'écran (regard, visibilité, éblouissement).

Réglages : *Max Angle* (40°) et *Power* (4) pour la forme du regard ; *Rise Speed* / *Fall Speed* pour le lissage ; *Strength* = 0 coupe l'effet (option du menu). Les traces suivent la **collision** (canal Visibility, complexe) : un objet sans collision ne cache pas le soleil.

## 7. Menu de réglages et sauvegarde (Blueprint)

1. **Struct** `S_DisplaySettings` : `Exposure` (float), `Tonemapper` (int), `UseSRGB` (bool), `Gamma` (float), `Brightness` (float), `SunGlareEnabled` (bool), plus les futurs réglages.
2. **SaveGame** `SG_Settings` : une variable `Display` de type `S_DisplaySettings`, avec les valeurs par défaut du tableau ci-dessus.
3. **Game Instance** (ex. `GI_Dwam`, déclarée dans *Project Settings → Maps & Modes*) :
   - fonction `ApplyDisplaySettings(S_DisplaySettings)` : pour chaque champ, **Set Scalar Parameter Value** sur `MPC_DwamPost` (bool → 0 / 1) ;
   - *Event Init* : **Does Save Game Exist** (`"Settings"`, 0) → **Load Game from Slot**, sinon **Create Save Game Object** (`SG_Settings`) ; garder la référence ; appeler `ApplyDisplaySettings`.
4. **Widget** du menu : curseurs et listes qui modifient une copie de la struct et appellent `ApplyDisplaySettings` (aperçu immédiat).
   - **Appliquer** : copier dans `SG_Settings.Display`, puis **Save Game to Slot** (`"Settings"`, 0).
   - **Annuler** : réappliquer `SG_Settings.Display`.
5. `SunGlareEnabled` : mettre *Strength* du composant à 0 ou 1 (le composant n'est pas dans la MPC).

La MPC n'existe qu'au moment où un monde tourne : appliquer les réglages à chaque chargement de map (ex. dans *BeginPlay* du Player Controller, en appelant la Game Instance).
