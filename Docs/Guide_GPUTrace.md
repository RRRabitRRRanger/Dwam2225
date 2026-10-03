# Guide — GPUTrace : utilisation, test du tuyau, montage Niagara

> Accueil : [[00 - Accueil]] · Plan : [[Plan_GpuTraces]]

> État (2026-10-02) : plugin `Plugins/GPUTrace` **compilé**, **pas encore testé en jeu**. Le moteur CPU marche sans aucun asset ; le moteur GPU demande le système Niagara décrit en partie 3.

## 1. Utilisation en Blueprint (moteur CPU, tout de suite)

Le moteur par défaut est le **CPU** : traces asynchrones d'Unreal **en mode complexe**, c'est-à-dire sur les triangles réels des meshes (la géométrie visuelle), pas sur les boîtes de collision simples.

### Le nœud « Async GPU Trace »

Clic droit dans un graphe Blueprint, chercher **Async GPU Trace** :

- entrées : `Start`, `End`, `Draw Debug` ;
- sorties : **On Hit** et **On Miss**, avec un `Result` (struct) à décomposer : `bHit`, `Location`, `Normal`, `Distance`, `Hit Actor` (CPU seulement), `Backend`, `Latency Frames`, `bTimedOut`.

Test minimal dans un Level Blueprint :

1. `Event BeginPlay` → `Async GPU Trace` (Start = un point, End = un point 10 m plus loin, Draw Debug coché).
2. `On Hit` → `Print String` avec `Result.Hit Actor` et `Result.Distance`.
3. `On Miss` → `Print String` « rien ».

### Les fonctions du sous-système

`Get GPUTrace Subsystem` (World Subsystem), puis :

- `Request Trace` (Start, End, un événement de retour, Draw Debug) → renvoie un identifiant ;
- `Set Backend` (CPU ou GPU Niagara), `Get Backend`, `Get Active Backend` (le moteur réellement utilisé : sans système Niagara configuré, le GPU retombe sur le CPU) ;
- `Get Pending Count`.

### Debug

- Paramètre `Draw Debug` de chaque trace.
- Console : `GPUTrace.Debug 1` dessine **toutes** les traces : vert = libre, rouge = impact, point jaune = impact, trait bleu = normale, point magenta = résultat jamais revenu (GPU).
- Durée d'affichage : *Project Settings > Plugins > GPU Trace > Debug Draw Duration*.

### Réglages

*Project Settings > Plugins > GPU Trace* : moteur par défaut, canal et mode complexe du CPU, système Niagara et nombre de slots du GPU, délai d'expiration.

## 2. Test du tuyau avec l'agent Bug

L'acteur **GPU Trace Bug Agent** (chercher « GPU Trace Bug Agent » dans *Place Actors*) rejoint un but en ne « voyant » le monde que par des traces :

- il avance vers le but tant que la voie est libre ;
- sinon il longe la paroi (palpe vers la paroi, puis tout droit, puis s'en écarte) ;
- il repart vers le but dès que la voie est libre et qu'il est plus proche qu'au point de contact (critère de l'algorithme Bug2, adapté à la 3D de façon heuristique).

Il se déplace **sans physique** : sa propre boîte de collision (`TrapBox`) ne le bloque pas.

Montage de la scène :

1. Un tuyau (Static Mesh) dont la **collision simple** est une boîte pleine. Dans l'éditeur de mesh : *Collision > Remove Collision*, puis *Add Box Simplified Collision*. Les traces complexes, elles, voient l'intérieur.
2. Placer l'agent **dans** le tuyau.
3. Déplacer le losange `GoalOffset` (handle du viewport) **hors** du tuyau, ou renseigner `Goal Actor`.
4. Jouer. À l'écran : messages à chaque changement d'état (« Vers le but », « Longe la paroi », « Arrivé », « Bloqué ») ; dans le viewport : rayons, chemin vert, sphère jaune du but, flèche orange de suivi de paroi.

Réglages utiles : `Step Size` (20 cm), `Agent Radius` (distance gardée aux parois), `Follow Right` (côté de contournement), `Max Cycles`.

Pour comparer : une trace classique en Blueprint (`Line Trace By Channel` **sans** *Trace Complex*) s'arrêtera sur la boîte du tuyau.

Événement Blueprint : `On Mode Changed` (sur l'agent) pour réagir à « Arrivé » ou « Bloqué ».

## 3. Moteur GPU : montage du système Niagara

> Je ne peux ni créer ni tester d'assets : ces étapes sont écrites d'après le source du moteur 5.8 (noms de fonctions vérifiés), mais le montage n'a jamais été essayé. À valider ensemble au premier essai.

Principe : un système persistant, **une particule = un slot de requête**. Les requêtes arrivent par trois tableaux en paramètres utilisateur ; chaque particule lance sa trace quand l'identifiant de son slot augmente, lit le résultat la frame suivante et l'**exporte** vers le C++ (data interface *Export*).

### 3.1 Le système

1. Créer un système Niagara vide, par exemple `NS_GPUTrace`, avec un émetteur vide.
2. **Émetteur** : *Sim Target* = **GPUCompute Sim** ; *Calculate Bounds Mode* = **Fixed**, avec des bornes très grandes (le système ne doit jamais être coupé pour visibilité).
3. **Système** : pas d'*Effect Type* qui coupe hors écran (aucun, ou un type sans culling).
4. **Paramètres utilisateur** (noms exacts, le C++ les cherche) :

| Nom | Type | Rôle |
|---|---|---|
| `GPUTrace_Starts` | Array Position | Départs des rayons, un par slot |
| `GPUTrace_Ends` | Array Position | Arrivées |
| `GPUTrace_Ids` | Array Int32 | Identifiant de requête par slot (il augmente à chaque nouvelle requête) |
| `GPUTrace_Slots` | Int32 (défaut 256) | Nombre de slots = nombre de particules |
| `GPUTrace_Callback` | Object | Reçoit le récepteur C++ des résultats |

### 3.2 Data interfaces (paramètres d'émetteur)

- **Async Gpu Trace** : *Max Traces Per Particle* = 1 ; *Trace Provider* = **HW Ray Tracing** (le seul qui voit le terrain PatchTerrain ; le Global SDF ne voit que les meshes avec MDF).
- **Export** : *Callback Handler Parameter* = `User.GPUTrace_Callback` ; *GPU Allocation Mode* = Fixed Size, *GPU Allocation Fixed Size* ≥ nombre de slots (256).

### 3.3 Spawn

- **Emitter Update** : *Spawn Burst Instantaneous*, *Spawn Count* = `User.GPUTrace_Slots`, *Spawn Time* = 0. Une seule fois.
- **Particle Spawn** : nouvelles variables
  - `Particles.SlotIndex` (int) = **Execution Index** ;
  - `Particles.LastIssuedId` (int) = 0 ;
  - `Particles.PendingId` (int) = -1.
- **Pas** de module *Particle State* ni de durée de vie : les particules vivent tant que le système vit.

### 3.4 Particle Update : un module Scratch Pad « GPUTrace Step »

Dans l'ordre :

**a) Lire le résultat de la frame précédente**

- `Read Async Ray Trace GPU` (data interface Async Gpu Trace) :
  - *Execute* = `PendingId >= 0`
  - *Previous Frame Query ID* = `SlotIndex`
  - sorties : *Collision Valid*, *Collision Pos World*, *Collision Normal*
- `Export Particle Data` (data interface Export) :
  - *Store Data* = `PendingId >= 0`
  - *Position* = *Collision Pos World* (converti en vecteur)
  - *Size* = `float(PendingId)`
  - *Velocity* = *Collision Valid* ? *Collision Normal* : (0, 0, 0)

  **Convention lue par le C++** : `Size` = identifiant de requête, `Velocity` = normale (zéro = raté), `Position` = impact.

**b) Lancer la nouvelle trace si le slot a une nouvelle requête**

- `Get` sur `User.GPUTrace_Ids`, `User.GPUTrace_Starts`, `User.GPUTrace_Ends` à l'index `SlotIndex` → `Id`, `Start`, `End`.
- `bNew` = `Id > LastIssuedId`.
- `Issue Async Ray Trace GPU` :
  - *Execute* = `bNew`
  - *Query ID* = `SlotIndex`
  - *Trace Start World* = `Start`, *Trace End World* = `End`
  - *Collision Group* = défaut

**c) Mettre à jour les variables (Map Set)**

- `PendingId` = `bNew` ? `Id` : -1
- `LastIssuedId` = `bNew` ? `Id` : `LastIssuedId`

### 3.5 Brancher

*Project Settings > Plugins > GPU Trace* : `Niagara System` = `NS_GPUTrace`, `Niagara Slots` = 256 (même valeur que `GPUTrace_Slots`), `Default Backend` = GPU (Niagara), ou `Set Backend` en Blueprint.

### 3.6 Vérifier

- `GPUTrace.Debug 1`, puis le test du tuyau en GPU : le nœud renvoie `Backend = GPU (Niagara)` et `Latency Frames` vaut 2 à 3.
- Des points **magenta** = résultats jamais revenus : vérifier les noms des paramètres utilisateur, le *Callback Handler Parameter* de l'Export, la taille d'allocation de l'Export et le *Trace Provider*.
- Log `LogGPUTrace` : « système Niagara … actif (256 slots) » au premier usage, ou l'explication du repli sur le CPU.
