# Plan — Passage en forward shading

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Lights]] · [[Plan_LightCulling]] · [[Plan_PostProcess]] · [[Plan_NormalTransfer]] · [[Guide_OKLab]]

> Statut : **décision de l'auteur** (2026-10-12 : « il faut à tout prix passer en forward, dès que possible »). **Réglages appliqués le 2026-10-12** dans `Config/DefaultEngine.ini`, avec son accord (« tu pourrais faire tous ces réglages ») : forward activé, Lumen et Nanite coupés, **MSAA 4x** (`r.AntiAliasingMethod=3`, demandé par l'auteur ; à rendre réglable dans le futur menu, voir [[Plan_PostProcess]]). Point de retour : commit `dec4f85`. **Pas encore ouvert dans l'éditeur.**

## ⚠️ À vérifier (auteur, quand il aura le temps)

Noté à la demande de l'auteur le 2026-10-12 ; rien de tout ceci n'a été contrôlé. Dans `Map_SHader_OKLCH_01a`, après la recompilation des shaders :

- [x] Le forward est actif (auteur, 2026-10-12 : plus de Buffer Visualization).
- [x] Le GDF est toujours généré (auteur : visible dans *Show → Visualize → Global Distance Field*).
- [x] Les ombres portées en ray-march sur le GDF : corrigées dans `Untitled_2` (auteur, 2026-10-12) ; `Gym_Shadow` reste à revoir avec le réglage. Historique : **cassées dans `Gym_Shadow`** (auteur, 2026-10-12 : « avec le bon angle tout devient noir » ; pas vues depuis la migration en 5.8 ; le MSAA n'y est pour rien). **Cause probable trouvée ensuite, voir « Le GDF s'éteint » plus bas.** ~~Première hypothèse de Claude~~ (peu probable : dans `Untitled_2`, les ombres tombent juste loin de l'origine) : sans lien avec le forward. Les nœuds Custom de `SHad_ShadowTest` appellent `GetDistanceToNearestSurfaceGlobal(p)` avec une position monde absolue, alors que la surcharge `float3` de 5.8 attend une position *translated world*, relative à la caméra (`GlobalDistanceFieldShared.ush:214`). Correction proposée : `GetDistanceToNearestSurfaceGlobal(DFFastToTranslatedWorld(p, ResolvedView.PreViewTranslation))`. Même piège à surveiller dans `MF_Light2` et `Lighting.ush`.
- [ ] Aucun matériau en erreur dans l'Output Log (« GBuffer scene textures not available with forward shading »).
- [ ] Le transfert de normales (`MF_Light2`) rend comme avant.
- [ ] Le post-process maison rend comme avant, MSAA compris.
- [ ] Les lumières `Dwam Light` sont visibles.
- [ ] Le MSAA 4x convient (bords de géométrie lissés ; le crénelage produit par le shader ne l'est pas).
- [ ] Pas de mesh qui a perdu du détail à cause de Nanite coupé.

## Le GDF s'éteint quand aucun matériau ne le « déclare » (2026-10-12)

**Symptôme** (captures de l'auteur, map `Untitled_2`) : tous les objets en `Shder_Redone` passent au noir, sauf quand un objet en `Shder_Windwaker` ou `Shder_Windwaker1` est à l'écran. Le sol (matériau par défaut) ne change pas.

**Cause** (lue dans le source 5.8 ; **confirmée par l'auteur le 2026-10-12** : la commande de la correction 1 règle le problème ; elle est inscrite dans `DefaultEngine.ini`, section `[SystemSettings]`, à revérifier après un redémarrage de l'éditeur) :

- Le moteur ne met à jour le Global Distance Field que si quelque chose le réclame : `ShouldPrepareGlobalDistanceField` (`DistanceFieldAmbientOcclusion.cpp:764`). Avant, Lumen le réclamait en permanence. Lumen coupé et forward activé, il ne reste que les matériaux visibles marqués `bUsesGlobalDistanceField`.
- Ce drapeau n'est posé que par les nœuds standard *DistanceToNearestSurface*, *DistanceFieldGradient* et *DistanceFieldApproxAO* (`HLSLMaterialTranslator.cpp:12179`). Un nœud Custom qui appelle `GetDistanceToNearestSurfaceGlobal` directement ne le pose pas.
- `Shder_Windwaker` utilise ces nœuds ; `Shder_Redone` passe par ses nœuds Custom. Sans Windwaker à l'écran, le GDF n'est plus préparé, la fonction renvoie 0 (aucun clipmap), le ray-march voit un obstacle partout : tout est à l'ombre.

**Corrections possibles**

1. Projet entier : `r.AOGlobalDistanceField.DetailedNecessityCheck 0` (le GDF est alors toujours préparé). À tester dans la console, puis à inscrire dans `DefaultEngine.ini`, section `[SystemSettings]`.
2. Par matériau : brancher un vrai nœud *DistanceToNearestSurface* sur une entrée du nœud Custom, pour que le matériau déclare lui-même son usage du GDF.

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
| `r.AntiAliasingMethod` | `0` (aucun) | `3` (MSAA, décision de l'auteur du 2026-10-12) | Le MSAA devient possible ; `r.MSAACount=4` est déjà réglé |
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
