# Plan — GPUTrace : traces sur la géométrie visuelle (GPU via Niagara, ou CPU)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_PatchTerrain]], [[Plan_NiagaraRope]], [[Plan_Water]]

> Statut : **validé** (2026-10-02), implémentation en cours. API vérifiées dans le source du moteur 5.8.

## Objectif

Faire beaucoup de traces (des centaines à des milliers par frame) sur le GPU plutôt qu'en Blueprint sur le CPU, et rendre leurs résultats au gameplay avec quelques frames de retard (accepté par l'auteur).

Usages pressentis (à prioriser avec l'auteur) :

- lecture du sol (pas, glissade, hoverboard) ;
- perception de l'IA (lignes de vue) ;
- humidité et impacts (pluie, éclaboussures) ;
- effets (étincelles, débris).

## Briques existantes dans Niagara 5.8

| Brique | Rôle | Remarques |
|---|---|---|
| **Async GPU Trace** (data interface) | Rayons depuis le GPU : `IssueAsyncRayTraceGpu` / `CreateAsyncRayTraceGpu` / `ReserveAsyncRayTraceGpu`, puis `ReadAsyncRayTraceGpu` la frame suivante (validité, distance, position, normale) | GPU uniquement. Réglages : `MaxTracesPerParticle`, `MaxRetraces`, `TraceProvider`. Groupes de collision pour filtrer |
| Fournisseur **HW Ray Tracing** | Trace dans la scène de ray tracing | Activé dans le projet (`r.Lumen.HardwareRayTracing=True`). **Voit le terrain PatchTerrain** (meshes dynamiques en ray tracing par défaut) |
| Fournisseur **Global SDF** | Ray march dans le Global Distance Field | **Ne voit pas le terrain PatchTerrain** (pas de MDF). Précis à environ 20 cm près de la caméra, se dégrade au loin |
| **Collision Query** (data interface) | `QuerySceneDepthGPU`, `QueryCustomDepthGPU`, `QueryMeshDistanceFieldGPU` : réponse immédiate, sans latence | Le depth buffer ne voit que ce qui est à l'écran ; le distance field a les mêmes limites que ci-dessus |
| **Data Channel Write** avec « Visible to Blueprint » | Renvoie au code de jeu (C++ ou Blueprint) des données écrites sur le GPU, avec une charge utile libre | C'est le bon chemin GPU → jeu |
| **Export** (data interface) | Alternative simple : renvoie position, taille et vitesse (7 floats) à un objet `INiagaraParticleCallbackHandler` | Plafonné à 1000 éléments par frame par défaut (`fx.Niagara.NDIExport.GPUMaxReadbackCount`) |

## Architecture initiale (~~Data Channels~~ : remplacée par « Implémentation retenue » ci-dessous)

```
Gameplay ──(requête : début, fin, filtre, id)──► Service C++ ──► Data Channel « Requêtes »
                                                                      │
                                       Système Niagara persistant (GPU) : 1 particule = 1 rayon
                                       frame N : IssueAsyncRayTraceGpu
                                       frame N+1 : ReadAsyncRayTraceGpu
                                                                      │
Gameplay ◄──(callback par id)── Service C++ ◄── Data Channel « Résultats » (Visible to Blueprint)
```

- **Service** : un `UWorldSubsystem` C++ (nom à choisir, neutre). Il expose une API Blueprint « demander une trace » (asynchrone, avec un délégué de retour), regroupe les requêtes d'une frame et redistribue les résultats par identifiant.
- **Latence** : requête à la frame N, résultat GPU à N+1, retour au jeu vers N+2 ou N+3.
- **Filtrage** : les groupes de collision de Niagara peuvent servir les échelles Macro / Méso / Micro (ex. ignorer le Micro pour la lecture du sol). À concevoir avec l'auteur.

## Décisions de l'auteur (2026-10-02)

- Nom : **GPUTrace**.
- Usage d'abord en **Blueprint** (Print String), avec une **visualisation de debug**.
- Scène de test de l'auteur : un objet caché dans un tuyau, « piégé » par la boîte de collision simple du tuyau, qui en sort grâce aux traces sur la géométrie visuelle.
- Pas de trajectoires balistiques.
- Démonstration de navigation : un **algorithme Bug** (robotique), simple.

## Implémentation retenue

- **Deux moteurs, une seule API** :
  - **CPU** : traces asynchrones d'Unreal en mode complexe (les triangles réels des meshes, donc la géométrie visuelle, pas les boîtes simples). Fonctionne sans aucun asset ; résultat à la frame suivante.
  - **Niagara (GPU)** : un système persistant, une particule par « slot » de requête. Les requêtes arrivent par des tableaux en paramètres utilisateur (`Array Position`, `Array Int32`) ; les traces passent par **Async GPU Trace** ; les résultats reviennent par le data interface **Export** (position, normale, identifiant de requête). Pas de Data Channel à créer. Montage décrit dans `Guide_GPUTrace.md`.
- **Blueprint** : un nœud asynchrone « Async GPU Trace » (sorties *On Hit* / *On Miss* avec le résultat), et les fonctions du sous-système (`Request Trace` avec un événement de retour, choix du moteur).
- **Debug** : lignes vertes (libre) / rouges (impact) avec point et normale ; variable console `GPUTrace.Debug 1`.
- **Agent Bug** (`AGPUTraceBugAgent`) : sphère qui va vers un but ; quand un rayon touche un obstacle, elle longe la paroi (règle de la main droite dans le plan de la paroi) jusqu'à pouvoir repartir vers le but en étant plus proche qu'au point de contact (critère de Bug2). En 3D, c'est une adaptation heuristique de Bug2. Elle se déplace sans physique (seules les traces comptent) et ignore sa propre boîte de collision.

## Répartition du travail

| Qui | Quoi |
|---|---|
| **Claude** | Le service C++ (plugin), son API Blueprint, la documentation des charges utiles, le HLSL éventuel, un pas-à-pas de montage |
| **Auteur** (assets dans l'éditeur) | Les deux Data Channels et le système Niagara, en suivant le pas-à-pas ; puis les tests |

## Points d'attention

- **Fournisseur** : HW Ray Tracing par défaut, puisque c'est le seul qui voit le terrain. Les acteurs exclus du ray tracing ne seront pas touchés.
- **Latence** : acceptable pour la perception, les pas et les effets ; pas pour ce qui doit répondre dans la frame (ex. tir instantané). Ces cas-là restent sur les traces CPU classiques.
- **Coût** : quelques milliers de rayons par frame restent légers pour le GPU. Le retour vers le jeu doit rester compact (seulement les résultats utiles).

## Étapes proposées

1. Prototype minimal : un système Niagara qui trace depuis des particules et affiche les impacts (aucun retour au jeu).
2. Data Channel « Résultats » + lecture en C++/Blueprint.
3. Service complet (requêtes depuis le gameplay, identifiants, callbacks).
4. Premier usage réel (à choisir : lecture du sol ou perception IA).

## Questions ouvertes

- Quels usages réels après la démo (lecture du sol, perception IA) ?
- ~~Nom~~ → **GPUTrace**. ~~Trajectoires courbes~~ → non.
