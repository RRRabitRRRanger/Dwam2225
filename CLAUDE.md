# CLAUDE.md — Dwam2225

Projet perso Unreal Engine. Réponds et commente en français.

> **Avant tout travail : lire `Docs/00 - Accueil.md`.** `Docs/` est le coffre Obsidian confié aux sessions Claude : plans par système, état d'avancement, décisions et journal. À tenir à jour en fin de session (règles dans l'accueil).

## Environnement

- **Moteur** : Unreal Engine **5.8.3** (build Launcher, non modifiable), installé dans `C:\Program Files\Epic Games\UE_5.8`
- **IDE** : Visual Studio Community 2022 (17.14), toolchain MSVC v143, Windows 11
- **Projet** : `D:\Nos Documents\Doc Alexandre\Unreal Projects\Dwam2225 5.8\Dwam2225.uproject` (le chemin contient des espaces : toujours le mettre entre guillemets)
- **Versioning** : Git + Git LFS (`.uasset` / `.umap` en LFS), GitHub Desktop

## État actuel

- Projet migré de 5.7 vers 5.8, puis passé en C++.
- Module de jeu : `Source/Dwam2225/` — contient seulement une classe vide (`PassDwam_To_C`) créée pour activer le C++.
- Plugin installé : Visual Studio Integration Tool (`Plugins/VisualStudioTools`).

## Système d'éclairage custom (le cœur du projet)

- Matériaux **unlit** avec éclairage calculé à la main (nœuds Custom / HLSL).
- Rendu non photoréaliste mais **sans bandes** : contrôle de la hardness, couleur d'ombre, dégradé, liseré coloré au terminateur ombre/lumière.
- Couleurs travaillées en **OKLab / OKLCH**.
- Ombres portées calculées en **ray-march sur les Mesh Distance Fields / Global Distance Field**.
- Données des lumières : actuellement via Material Parameter Collection, en cours de migration vers une **texture N×1 float RGBA**, 2 pixels par lumière, type de lumière encodé en bitwise.

## Objectifs à venir

1. Créer un plugin de projet **`Plugins/DwamLighting/`** :
   - `Source/` : composant qui collecte les vraies `ULightComponent` (lumières « fantômes » avec `bAffectsWorld = false`) et remplit la texture (`UpdateTextureRegions`).
   - `Shaders/` : `.ush` / `.usf` du système, mappés via `AddShaderSourceDirectoryMapping` dans `StartupModule` (module en phase `PostConfigInit`, dépendance `RenderCore`).
   - `Content/` : matériaux de base du système.
2. **Culling des lumières** : reproduire une light grid (froxels) dans une **Scene View Extension** + compute shader, en s'inspirant de `Engine/Source/Runtime/Renderer/Private/LightGridInjection.cpp` et des shaders de light grid du moteur. La grille interne d'Unreal n'est pas accessible aux matériaux (déjà testé).

## Règles

- **Ne jamais modifier le moteur** (`C:\Program Files\Epic Games\...`). Tout passe par le projet ou des plugins.
- **Ne jamais éditer, créer ou supprimer de `.uasset` / `.umap`** : ce sont des binaires gérés par l'éditeur. Si un asset doit changer, explique-moi quoi faire dans l'éditeur.
- Ne pas déplacer ou renommer de dossiers dans `Content/` depuis le système de fichiers (les références casseraient) : ça se fait dans l'éditeur.
- Le code du système d'éclairage va dans `Plugins/DwamLighting/`, pas dans le module de jeu.
- Avant un changement important, écris d'abord un plan dans `Docs/` et attends ma validation.
- Ne pas modifier `.gitignore`, `.gitattributes` ni faire de commit sans me demander.
- Suivre les conventions Unreal : préfixes `A` (Actor), `U` (UObject), `F` (struct), `E` (enum), `I` (interface), `T` (template) ; macros `UCLASS` / `UPROPERTY` / `UFUNCTION`.

## Compiler

L'éditeur doit être fermé, sinon Live Coding bloque la compilation (ou compiler depuis l'éditeur avec Ctrl+Alt+F11).

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Dwam2225Editor Win64 Development -Project="D:\Nos Documents\Doc Alexandre\Unreal Projects\Dwam2225 5.8\Dwam2225.uproject" -WaitMutex
```

Régénérer les fichiers de projet VS : clic droit sur le `.uproject` → *Generate Visual Studio project files*.

## Bugs connus en 5.8 à garder en tête

- Substrate Toon Profile : l'override dans un Material Instance n'est pas appliqué après redémarrage de l'éditeur tant que le shader n'est pas recompilé.
