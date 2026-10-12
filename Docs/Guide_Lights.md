# Guide — Lumières maison (Dwam Light)

> Accueil : [[00 - Accueil]] · Plan : [[Plan_Lights]] · Lié à : [[Guide_OKLab]] · [[Plan_LightCulling]]

> État (2026-10-11) : le code compile, les tests du packing passent, `Lights.ush` est validé par compilation hors moteur. **Rien n'a encore été vu dans l'éditeur** : ce guide est le premier essai. Si une étape ne donne pas le résultat annoncé, c'est une information utile, pas une erreur de manipulation.

> **2026-10-12** : premier essai par l'auteur, en forward. Render target créé, matériau de test monté : **vert dans la scène**. Le transport fonctionne (texture écrite, entiers intacts). Puis une `Dwam Light` posée et un matériau branché sur `DwamAccumulateLights` : **la lumière éclaire la surface** (« trop bien ! »). Confirmé ensuite par l'auteur (« tout ça fonctionne ») : la tache reste collée quand la caméra bouge, spot et rect, plusieurs lumières qui s'additionnent, layers.

## 1. Créer le render target (une fois)

1. *Content Browser* → réglages (en haut à droite) → cocher **Show Plugin Content**.
2. Aller dans **Plugins > DWAM Lighting Content**.
3. Clic droit → *Textures* (ou *Materials & Textures*) → **Render Target**. Le nommer exactement `RT_DwamLights`.
4. L'ouvrir et régler :

| Réglage | Valeur |
|---|---|
| Size X | 256 |
| Size Y | 1 |
| Render Target Format | **RTF RGBA32f** |
| Address X / Address Y | Clamp |
| Texture Filter (catégorie *Texture*) | Nearest |
| Auto Generate Mips | décoché |

5. Enregistrer.

Le plugin le cherche à cet emplacement par défaut. Pour un autre nom ou un autre dossier : *Project Settings > Plugins > DWAM Lighting > Lights Render Target*.

- 256 pixels = 127 lumières. Le budget par image se règle au même endroit (*Max Lights*, 64 par défaut).
- Tant que l'asset n'existe pas, le log affiche un avertissement `LogDwamLighting` (une fois) et le plugin réessaie toutes les 2 secondes : pas besoin de redémarrer l'éditeur après l'avoir créé.

## 2. Vérifier le transport (le premier test à faire)

Un matériau **Unlit** de debug, à poser sur n'importe quel mesh :

1. Nœud **Texture Object**, texture = `RT_DwamLights`. Si l'éditeur le demande, *Sampler Type* = **Linear Color**.
2. Nœud **Custom** :
   - *Output Type* : CMOT Float 3 ;
   - *Include File Paths* : `/Plugin/DwamLighting/Lights.ush` ;
   - une entrée nommée `LightsTex`, reliée au Texture Object ;
   - code :

```hlsl
return DwamLightsSelfTest(LightsTex);
```

3. Sortie du Custom → *Emissive Color*.

| Couleur | Ce que ça veut dire |
|---|---|
| **Vert** | la texture arrive, et les entiers passent intacts par le GPU. Tout va bien. |
| **Rouge** | la texture arrive, mais des bits sont altérés en route. Me le dire : il faudra le repli par compute shader. |
| **Bleu** | la texture n'a pas été écrite (asset mal nommé, mauvais format, ou l'écriture ne marche pas). Regarder le log `LogDwamLighting`. |

Ça doit marcher dans le viewport de l'éditeur, sans lancer le jeu.

## 3. Poser des lumières

- *Place Actors* → chercher **Dwam Light** ; ou ajouter le composant **Dwam Light** à n'importe quel acteur ou Blueprint.
- Sélectionné, l'acteur montre son gizmo : sphère de portée (point), cônes (spot), rectangle et flèche du côté qui émet (rect).

| Réglage | Effet |
|---|---|
| Type | Point, Spot ou Rect |
| Light Color | couleur |
| Temperature (case à cocher) | multiplie la couleur par celle d'un corps noir |
| Intensity | **1 = pleine lumière au centre** |
| Range | distance à laquelle la lumière s'éteint complètement |
| Falloff Exponent | dureté du fondu : petit = large, grand = serré autour du centre |
| Layers | la lumière n'éclaire que les matériaux qui partagent un layer avec elle |
| Inner / Outer Cone Angle | spot : pleine lumière à l'intérieur, plus rien à l'extérieur |
| Source Width / Height | rect : taille du rectangle (Y = largeur, Z = hauteur ; X = côté qui émet) |
| Cast Shadow, Shadow Softness | transmis au shader, **pas encore utilisés** |

Une lumière cachée (œil fermé dans l'outliner, *Visible* décoché, *Enabled* décoché, intensité nulle) n'est pas envoyée.

## 4. Éclairer un matériau

Nœud **Custom**, *Output Type* CMOT Float 3, include `/Plugin/DwamLighting/Lights.ush`.

| Entrée | Brancher |
|---|---|
| `LightsTex` | Texture Object → `RT_DwamLights` |
| `MappedNormal` | la normale avec normal map, en monde |
| `VertexNormal` | `VertexNormalWS` |
| `ReceiverLayers` | un scalaire : 255 = tous les layers ; 1 = layer 0 seul ; 2 = layer 1 ; 3 = les deux… |

```hlsl
return DwamAccumulateLights(LightsTex, GetTranslatedWorldPosition(Parameters),
	MappedNormal, VertexNormal, 0.1, (uint)ReceiverLayers);
```

Le résultat est la **somme, en RGB linéaire, de toutes les lumières** (couleur × éclairement × N·L), sans le soleil et sans ombre.

- **Pour voir tout de suite** : `Albedo × résultat` dans *Emissive Color*.
- **Position** : toujours `GetTranslatedWorldPosition(Parameters)`. Les lumières sont stockées relativement à la caméra ; la position monde absolue donnerait n'importe quoi.

### Avec le diffus OKLab (proposition, à voir ensemble)

`DwamShadeDiffuseOKLab` attend un éclairement de 0 à 1. Une façon simple de le nourrir avec la somme RGB, `Lights` :

```hlsl
float  Local       = max(Lights.r, max(Lights.g, Lights.b));          // force des lumières locales
float3 Tint        = Lights / max(Local, 1e-4);                        // leur couleur, normalisée
float  LightFactor = saturate(SunFactor + Local);                      // soleil + lumières locales
float3 Lab         = DwamShadeDiffuseOKLab(AlbedoLab, LightFactor, /* réglages */ ...);
float3 Rgb         = DwamOKLabToLinearSRGB(Lab);
Rgb *= lerp(1.0, Tint, saturate(Local / max(SunFactor + Local, 1e-4))); // teinte, au prorata de leur part
```

C'est un point de départ, pas une décision : le liseré et le scotopique réagissent à la somme, et la couleur des lumières teinte le résultat là où elles dominent.

## 5. Voir le coût

```hlsl
return DwamLightsInRange(LightsTex, GetTranslatedWorldPosition(Parameters)) / 8.0;
```

Blanc = 8 lumières ou plus atteignent ce pixel. C'est ce chiffre que le culling ([[Plan_LightCulling]]) ne changera pas ; il réduira le nombre de lumières *examinées*, aujourd'hui toutes.

## En cas de problème

| Symptôme | Piste |
|---|---|
| Le test reste bleu | nom ou dossier de l'asset ; format autre que RTF RGBA32f ; message `LogDwamLighting` dans l'*Output Log* |
| Lumières décalées, qui suivent la caméra | position monde absolue au lieu de `GetTranslatedWorldPosition` |
| Une lumière n'éclaire pas | layers (lumière et matériau n'en partagent aucun) ; lumière cachée ; `ReceiverLayers` à 0 |
| Des lumières disparaissent | budget dépassé : avertissement dans le log ; monter *Max Lights* ou élargir le render target |
| Le `.ush` modifié n'est pas pris en compte | console : `recompileshaders changed` |
| L'aperçu du matériau (la sphère) est noir | normal : le monde de l'aperçu n'a aucune lumière maison |

## Lancer les tests

*Session Frontend > Automation* → filtre `DwamLighting` ; ou, éditeur fermé :

```
UnrealEditor-Cmd.exe <projet> -ExecCmds="Automation RunTests DwamLighting; Quit" -nullrhi -unattended
```
