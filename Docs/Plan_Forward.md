# Plan — Passage en forward shading

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Lights]] · [[Plan_LightCulling]] · [[Plan_PostProcess]] · [[Plan_NormalTransfer]] · [[Guide_OKLab]]

> Statut : **décision de l'auteur** (2026-10-12 : « il faut à tout prix passer en forward, dès que possible »). Ce qui attend sa validation : la liste exacte des réglages ci-dessous et les quatre questions de la fin. **Rien n'est modifié dans le projet** : `Config/DefaultEngine.ini` est intact.

## Pourquoi (auteur, 2026-10-11)

Un rendu photoréaliste « à l'ancienne » : pas de Lumen, pas de Nanite. Le forward le faisait très bien, sans le brouillard et les tonemappers qui aplatissent l'image jusqu'à ce que plus aucun détail ne ressorte (« c'est tout gris uni »).

Lecture de Claude : le forward va aussi dans le sens du système. Les matériaux sont unlit et calculent leur éclairage eux-mêmes ; en deferred, le moteur alloue et remplit un GBuffer qui ne sert à rien.

## Ce que le forward change (vérifié dans le source 5.8)

| Sujet | Effet | Source |
|---|---|---|
| Activation | `r.ForwardShading` est en lecture seule : redémarrage de l'éditeur et **recompilation de tous les shaders** | `RenderUtils.cpp:737` |
| Nanite | **Désactivé** en forward. Les meshes Nanite s'affichent avec leur fallback mesh | `RenderUtils.cpp:1430` |
| Textures de scène du GBuffer | Un matériau qui lit `SceneTexture : WorldNormal`, `BaseColor`, `Roughness`, etc. **ne compile plus** (« GBuffer scene textures not available with forward shading ») | `HLSLMaterialTranslator.cpp:8323` |
| Global Distance Field | Pas de blocage lié au forward : il est préparé dès qu'un matériau ou Niagara le lit, à condition que le Distance Field AO reste supporté (`r.DistanceFieldAO`, activé par défaut) | `DistanceFieldAmbientOcclusion.cpp:764-811` |
| Sky light dynamique en passe deferred | Désactivé | `DistanceFieldAmbientOcclusion.cpp:711` |
| Reflets screen space (SSR), GI screen space | Désactivés | `CompositionLighting.cpp:70`, `ScreenSpaceRayTracing.cpp:127` |
| SSAO | Calculé avant la base pass, à partir de la profondeur seule | `CompositionLighting.cpp:648` |
| Vélocité | Plus écrite dans la base pass ; passe séparée si besoin | `RenderUtils.cpp:1702` |
| Depth prepass complet | Forcé (le projet l'a déjà : `r.EarlyZPass=3`) | `RenderUtils.cpp:700` |
| Volumes hétérogènes, filtrage VSM de l'eau | Désactivés | `RenderUtils.cpp:1541`, `1556` |
| Limites annoncées par Epic | Une seule reflection capture par objet ; pas d'ombres dynamiques reçues par les translucides | description de `r.ForwardShading` |
| MSAA | Devient disponible (`r.AntiAliasingMethod=3`) | — |

**Lumen** : le projet a `r.DynamicGlobalIlluminationMethod=1` (Lumen). Lumen ne fonctionne pas en forward d'après la documentation d'Epic ; je n'ai pas retrouvé la ligne dans le source (`À COMPLÉTER`). Sans incidence : le réglage passe à « None » de toute façon.

## Impact sur les systèmes du projet

| Système | Impact attendu |
|---|---|
| Matériaux unlit à éclairage maison (`MaterialBase`, `MF_Light2`) | Aucun en principe. **À contrôler** : aucun nœud `SceneTexture` du GBuffer dedans (je ne peux pas lire les `.uasset`) |
| Ombres en ray-march sur le GDF, transfert de normales ([[Plan_NormalTransfer]]) | Devraient tenir : le GDF reste préparé. **Premier point à vérifier dans l'éditeur** |
| Lumières maison ([[Plan_Lights]]) | Aucun : la Scene View Extension écrit le render target dans `PreRenderViewFamily_RenderThread`, hors du pipeline de shading |
| Culling par Z-binning ([[Plan_LightCulling]]) | Aucun : pas de dépendance au GBuffer. Le plan dit déjà « pas de Nanite, pas de TSR/TAA » |
| Post-process maison ([[Plan_PostProcess]]) | Aucun : il ne lit que `PostProcessInput0` et `SceneDepth`, disponibles en forward |
| GPUTrace, fournisseur HW Ray Tracing ([[Plan_GpuTraces]]) | Inchangé : `r.RayTracing` est déjà à `False` |
| PatchTerrain, Watershed, Rope, GraphKit, LayoutKit, LOA | Aucun (pas de rendu spécifique au deferred) |
| Meshes Nanite existants dans `Content/` | Fallback mesh affiché : possible perte de détail. Inconnu : `À COMPLÉTER` (y en a-t-il ?) |

## Réglages proposés

Dans *Project Settings → Engine → Rendering*, ou directement dans `Config/DefaultEngine.ini`, section `[/Script/Engine.RendererSettings]` :

| Réglage | Aujourd'hui | Proposé | Pourquoi |
|---|---|---|---|
| `r.ForwardShading` | `False` | `True` | La décision |
| `r.DynamicGlobalIlluminationMethod` | `1` (Lumen) | `0` (None) | Pas de Lumen |
| `r.Lumen.HardwareRayTracing` | `True` | `False` | Devient inutile ; évite de compiler des shaders pour rien |
| `r.Nanite.ProjectEnabled` | `True` | `False` (question 1) | Désactivé de fait en forward ; le couper évite de garder les données Nanite |
| `r.AntiAliasingMethod` | `0` (aucun) | question 2 | Le MSAA devient possible ; `r.MSAACount=4` est déjà réglé |
| `r.ReflectionMethod` | `0` | inchangé | Déjà sans Lumen |
| `r.VertexFoggingForOpaque` | `False` | inchangé | Brouillard par pixel conservé ; à revoir avec le brouillard maison |

## Étapes

1. **Point de retour** : commit de l'état actuel sur la branche (avec l'accord de l'auteur), pour pouvoir revenir en arrière en une commande.
2. Éditeur fermé : modifier les réglages (par Claude dans le `.ini`, ou par l'auteur dans les Project Settings).
3. Rouvrir l'éditeur : recompilation de tous les shaders (long la première fois ; le DDC du deferred reste en cache, le retour arrière sera rapide).
4. **Contrôles**, dans `Map_SHader_OKLCH_01a` :
   - aucun matériau en erreur dans l'Output Log (chercher « GBuffer scene textures not available ») ;
   - ombres portées en ray-march toujours présentes (sinon : `r.DistanceFieldAO 1`, `r.AOGlobalDistanceField 1`, et `ShowFlag.VisualizeGlobalDistanceField 1` pour voir le champ) ;
   - transfert de normales (`MF_Light2`) inchangé ;
   - post-process maison inchangé ;
   - lumières `Dwam Light` visibles.
5. Noter les résultats ici et dans le journal ; mettre à jour `CLAUDE.md` (« rendu forward »).

**Retour arrière** : remettre les valeurs de la colonne « Aujourd'hui » et redémarrer.

## Questions à l'auteur

1. **Nanite** : je le coupe dans le projet (`r.Nanite.ProjectEnabled=False`), ou je le laisse activé « au cas où » ? Des meshes du projet sont-ils en Nanite ?
2. **Anticrénelage** : aucun (comme aujourd'hui), FXAA, ou MSAA 4x ? Le MSAA ne lisse que les bords de géométrie, pas le crénelage produit par le shader (terminateur dur, motifs).
3. **Qui applique** : je modifie le `.ini` moi-même une fois le plan validé (éditeur fermé), ou tu passes par les Project Settings ?
4. **Commit avant** : d'accord pour un commit de l'état actuel sur `patchterrain-et-plans` comme point de retour ?
