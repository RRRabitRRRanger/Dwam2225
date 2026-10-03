# Guide — OKLab.ush et Lighting.ush (couleur et éclairage maison)

> Accueil : [[00 - Accueil]] · Lié à : [[Guide_NormalTransfer]]

> État (2026-10-03) : écrit à partir de la relecture complète du HLSL de `Content/AVOLABO/Shaders/OKLCH/` (`Shader_OKLCH`, `Func1`, `EyeLids`, `Eyes`, `HackLight`). Les deux bibliothèques sont **validées par compilation** (DXC SM6, FXC SM5, hors moteur). Le gamut mapping analytique est **validé numériquement** : erreur moyenne de chroma 0,00001, maximum 0,0008, aucun dépassement, sur 466 couleurs hors gamut. **Rien n'est encore testé dans l'éditeur.** Les matériaux de l'auteur ne sont pas modifiés.

## Les fichiers

| Fichier (chemin virtuel) | Contenu |
|---|---|
| `/Plugin/DwamLighting/OKLab.ush` | Conversions RGB ↔ OKLab ↔ OKLCH ; opérations en OKLab (chroma, rotation de teinte, mélange) ; gamut mapping analytique (Ottosson) + l'ancienne dichotomie pour comparer |
| `/Plugin/DwamLighting/Lighting.ush` | Garde-fou de normal map ; origine des rayons d'ombre ; diffus en OKLab ; tous les spéculaires |

Dans un nœud Custom : *Include File Paths* → ajouter `/Plugin/DwamLighting/OKLab.ush` et/ou `/Plugin/DwamLighting/Lighting.ush`. Les deux sont indépendants.

Une modification d'un `.ush` se teste **sans redémarrer** : Ctrl+Shift+. (ou `recompileshaders changed`).

## Pourquoi OKLab plutôt qu'OKLCH

Tes trois opérations d'éclairage s'écrivent directement en OKLab, pour le même résultat :

| Avant (OKLCH, à chaque pixel) | Maintenant (OKLab) |
|---|---|
| `C *= facteur` | `ab *= facteur` (aucune trigonométrie) |
| `H += shift`, puis `fmod` et le test < 0 | rotation de `ab` : un `sincos`, pas d'`atan2` ni de `fmod` |
| Teinte indéfinie sur les gris (`atan2` de presque zéro) | Un gris reste `(0, 0)` : aucun problème |

OKLCH reste utile pour l'**interface** (teinte en degrés) et pour **mélanger deux couleurs en gardant la saturation** (`DwamOKLabLerp(..., bHuePath = true)`, qui remplace le bloc « chemin le plus court » d'`EyeLids`).

## Correspondance : ancien bloc → nouvelle fonction

| Ancien bloc (Shader_OKLCH / Func1) | Remplacement |
|---|---|
| `Linear sRGB -> Oklch` | `DwamLinearSRGBToOKLab(RGB)` (rester en Lab) ; `DwamOKLabToOKLCH` seulement si tu as besoin des degrés |
| `Oklch -> Linear sRGB` (3 exemplaires, dont `Func1`) | `DwamOKLabToLinearSRGB(Lab)` (avec gamut) |
| `Oklch -> Linear sRGB (avec gamut mapping)` (dichotomie, 8 itérations) | `DwamOKLabToLinearSRGB(Lab)` (analytique, sans boucle) ; `DwamOKLabToLinearSRGBBisect(Lab)` garde l'ancienne méthode pour comparer |
| `ShadeDiffuseOklch` (L en énergie) | `DwamShadeDiffuseOKLab(...)` |
| `ShadeDiffuseOklch` (L linéaire, chroma 3 zones) | `DwamShadeDiffuseOKLabLinearL(...)` |
| Lerp de deux couleurs LCH, chemin court (`EyeLids`) | `DwamOKLabLerp(LabA, LabB, Alpha, true)` |
| Blinn-Phong + Schlick + terminateur doux | `DwamSpecular(2, …)` ou `DwamSpecular(3, …)` (normalisé) |
| Phong normalisé (Lazarov/Karis) | `DwamSpecular(1, …)` |
| Phong brut (Gloss × pow) | `DwamSpecular(0, …)` (avec F0 = 1 pour couper Fresnel) |

## Corrections incluses

1. **Scotopique sur l'éclairement, plus sur L.** Une version utilisait la clarté de l'objet : un rouge foncé en plein soleil se désaturait comme la nuit. Maintenant, seule la lumière reçue (`LightFactor`) décide.
2. **L en énergie** (racine cubique) par défaut : cohérent avec la physique, et indispensable pour **plusieurs lumières**. Il faudra additionner les éclairements (`LightFactor`) des lumières de la texture 1D, puis appeler le diffus **une seule fois**. Additionner des couleurs déjà stylisées serait faux.
3. **Plus de `max(rgb, 0)` qui fait dériver la teinte** : la conversion finale passe toujours par le gamut mapping, qui réduit la chroma à L et teinte constants.
4. **Code unique** : une fonction par conversion au lieu de copies (la matrice OKLab apparaissait 3 fois dans le seul bloc avec gamut).
5. **Spéculaire sans bande** : le masque au terminateur est toujours doux (`DwamSpecTerminator`), jamais un `step` dur.

## Le bug de la normal map (faces à l'ombre éclairées par le soleil)

**Cause** : la normal map penche la normale pixel par pixel. Juste derrière le terminateur, la vraie surface tourne déjà le dos à la lumière, mais la normale perturbée peut encore la regarder (`dot(N_map, L) > 0` alors que `dot(N_géo, L) < 0`). Des morceaux de face cachée s'allument.

**Correction** : deux normales.
- **Normal map** (`PixelNormalWS` ou ta normale déballée en monde) : la forme de l'éclairage.
- **Normale géométrique** (`VertexNormalWS`) : un garde-fou doux.

```hlsl
// Diffus : facteur d'éclairement protégé (Softness ≈ 0.1)
float NdotL = DwamGuardedNdotL(MappedNormal, VertexNormal, LightDir, 0.1);

// Spéculaire : même garde-fou
float Spec = DwamSpecular(Model, MappedNormal, LightDir, ViewDir, Power, F0, 0.25)
           * DwamNormalMapHorizon(VertexNormal, LightDir, 0.1);

// Ombres SDF : départ du rayon sur la normale GÉOMÉTRIQUE (jamais celle de la normal map)
float3 Start = DwamShadowRayOrigin(WorldPos, VertexNormal, LightDir, Bias);
```

`LightDir` = `SkyAtmosphereLightDirection` pour le soleil (normalisé).

## Spéculaire : tout est en stock

`DwamSpecular(Model, N, L, V, Power, F0, TerminatorSoftness)` :

| Model | Nom | Caractère |
|---|---|---|
| 0 | Phong | Tache ronde, intensité libre (règle-la avec ton Gloss) |
| 1 | Phong normalisé `(n+2)/2π` | Énergie constante : la tache s'intensifie en se resserrant |
| 2 | Blinn-Phong | Tache plus large et allongée aux angles rasants (plus « réelle ») |
| 3 | Blinn-Phong normalisé `(n+8)/8π` | Idem, énergie constante |

Astuce pour retrouver celui que tu préfères : mets `Model` en paramètre scalaire de ton matériau (0 à 3) et bascule en direct dans une instance de matériau. Note : les modèles normalisés donnent des valeurs bien plus fortes à `Power` élevé, donc il faut baisser l'intensité.

## Montage type (diffus OKLab complet)

Nœud Custom (CMOT Float 3), includes `OKLab.ush` et `Lighting.ush`, entrées : `Albedo` (RGB linéaire), `MappedNormal`, `VertexNormal`, `LightDir`, `Shadow` (ton ombre SDF 0..1), et les réglages.

```hlsl
float3 AlbedoLab = DwamLinearSRGBToOKLab(Albedo);
float LightFactor = DwamGuardedNdotL(MappedNormal, VertexNormal, LightDir, 0.1) * Shadow;
float3 Lab = DwamShadeDiffuseOKLab(AlbedoLab, LightFactor,
	AmbientEnergy, TerminatorPow, ChromaInLight, BoostCenter, BoostWidth, ChromaBoost,
	ScotopicEnd, ScotopicStr, HueShift, HueFalloff);
return DwamOKLabToLinearSRGB(Lab);
```

## Ce qui reste à faire (dans l'éditeur, par l'auteur)

1. Dupliquer un matériau OKLCH, remplacer un bloc à la fois, comparer avant et après.
2. Vérifier la normal map : les faces à l'ombre ne doivent plus s'allumer ; régler `Softness` (0,05 à 0,2).
3. Choisir son spéculaire avec le paramètre `Model`.

## Motifs procéduraux filtrés (`Patterns.ush`)

Include : `/Plugin/DwamLighting/Patterns.ush`. Validé par compilation (DXC et FXC).

**Principe** : au lieu de lire le motif au centre du pixel, ce qui scintille au loin, on calcule sa **moyenne exacte sur l'empreinte du pixel**, grâce aux dérivées écran. Ça marche sans mipmap et sans TAA (cohérent avec le choix de ne pas utiliser de TAA). Au loin, le motif se fond proprement en gris moyen. D'après Inigo Quilez, « Box filtered XOR pattern » (https://www.shadertoy.com/view/tdBXRW) ; licence du code d'origine à vérifier sur la page.

| Fonction | Motif |
|---|---|
| `DwamFilteredXor(P, ddx(P), ddy(P))` | XOR sur 8 octaves (la sphère de la capture) |
| `DwamFilteredChecker(P, ddx(P), ddy(P))` | Damier, une octave |
| `DwamFilteredXorAuto(P)` / `DwamFilteredCheckerAuto(P)` | Idem, dérivées calculées automatiquement (pixel shader seulement) |

Nœud Custom (CMOT Float 1), entrée `P` = `TexCoord × échelle` (ou une position monde divisée par la taille de case) :

```hlsl
return DwamFilteredXorAuto(P);
```

À brancher sur une sortie de pixel (Base Color, Emissive, ou ton albédo avant le diffus OKLab), **pas** sur World Position Offset : `ddx`/`ddy` n'existent pas dans le vertex shader.
