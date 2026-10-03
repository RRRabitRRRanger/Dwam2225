# Guide — Transfert de normales : matériaux et snake

> Accueil : [[00 - Accueil]] · Plan : [[Plan_NormalTransfer]]

> État (2026-10-02) : bibliothèque `Plugins/DwamLighting/Shaders/NormalTransfer.ush` **validée par compilation HLSL** (dxc SM6 et fxc SM5, hors moteur) ; acteur `Snake Toy` **compilé**. Rien n'a encore été testé dans l'éditeur.

## 0. Rendre la bibliothèque accessible

Le plugin `DwamLighting` (chargé en `PostConfigInit`) déclare le chemin virtuel `/Plugin/DwamLighting`. Dans chaque nœud Custom qui l'utilise :

- *Include File Paths* → ajouter **`/Plugin/DwamLighting/NormalTransfer.ush`**

La bibliothèque ne contient que des maths : tout ce qui vient du moteur (distance field, Custom Primitive Data, positions) se branche sur les entrées du nœud.

## 1. Variante « champ » (GDF) : n'importe quel mesh avec MDF

À utiliser sur les pièces de personnage, les rochers posés les uns contre les autres, etc.

### Nœuds à poser

| Nœud | Branchement |
|---|---|
| `ProbeDistance` (Scalar Parameter, ex. 15) | hauteur de sonde (cm) ≈ largeur du raccord voulu |
| `ProbePos` | `Absolute World Position` + `VertexNormalWS` × `ProbeDistance` |
| **Distance To Nearest Surface** | *Position* = `ProbePos` |
| **Distance Field Gradient** | *Position* = `ProbePos` |
| `Softness` (Scalar, ex. 0.5), `Strength` (Scalar, ex. 1) | réglages |
| `DebugShowMask` (Scalar, 0 ou 1) | affiche le masque de jonction |

Les deux nœuds GDF sont **indispensables** : c'est leur présence qui donne au matériau l'accès au Global Distance Field.

Utiliser `VertexNormalWS` pour la sonde (et pas la normale finale du pixel) évite une boucle dans le graphe.

### Nœud Custom « DwamNormalField »

- *Output Type* : **CMOT Float 3**
- *Include File Paths* : `/Plugin/DwamLighting/NormalTransfer.ush`
- Entrées : `N` (normale monde du pixel : `VertexNormalWS`, ou ta normale normal-mappée en monde), `ProbeDistance`, `FieldDistance` (sortie de Distance To Nearest Surface), `FieldGradient` (sortie de Distance Field Gradient), `Softness`, `Strength`, `DebugShowMask`
- Code :

```hlsl
float Mask;
float3 R = DwamNormalTransferField(normalize(N), ProbeDistance, FieldDistance, FieldGradient, Softness, Strength, Mask);
return lerp(R, Mask.xxx, DebugShowMask);
```

La sortie est une **normale monde** : à brancher dans l'entrée normale de ton éclairage maison (MF). Pour un test rapide en *Default Lit* : décocher **Tangent Space Normal** dans le matériau et brancher sur *Normal*.

### Limites

- Précision du GDF : environ 20 cm près de la caméra, moins au loin.
- Ne voit que les meshes avec MDF (pas le terrain PatchTerrain).
- Si `ProbeDistance` est trop grand, des objets voisins mais non collés « fusionnent » aussi : régler avec `DebugShowMask`.

## 1 bis. Variante « surface » : ta méthode (MF_Light2), en code

Gradient du GDF **à la surface**, mélangé à la normale du mesh avec un alpha global. Fusion des jonctions **et** arêtes convexes adoucies, partout.

- Nœuds : **Distance Field Gradient** avec *Position* = `Absolute World Position` (ou ton P actuel) ; l'alpha = ton calcul actuel (paramètre × `Ratioblob` × (1 − AO), etc.).
- Nœud Custom (CMOT Float 3, même include), entrées `N`, `FieldGradient`, `Alpha` :

```hlsl
return DwamNormalTransferSurface(normalize(N), FieldGradient, Alpha);
```

## 1 ter. Variante « hybride » : porter loin, sans AO, sans coller au sol

Visée : le mélange large de ta méthode, la sobriété de la variante champ, et pas d'AO.

- **Pourquoi ça porte loin** : le gradient pris à une hauteur R au-dessus de la surface est la normale de l'union des objets **arrondie de rayon R**. Une sonde haute (R2 ≈ 40 cm) donne une fusion large et organique.
- **Deux sondes** : une proche (R1 ≈ 5 cm, garde la forme) et une lointaine (R2 ≈ 40 cm, fusion large).
- **Base Blend** : une part de mélange partout, comme ta méthode (tu peux y brancher `Ratioblob` pour un réglage par objet).
- **Junction Strength** : un renfort aux jonctions, détecté par les sondes elles-mêmes (pas d'AO).
- **Ground Rejection** : réduit la fusion sous un objet posé (pixel orienté vers le bas dont la normale fusionnée pointe vers le haut). C'est une heuristique, à régler à l'œil.

Nœuds : deux fois le couple *Distance To Nearest Surface* + *Distance Field Gradient*, l'un en `WorldPos + VertexNormalWS × R1`, l'autre en `WorldPos + VertexNormalWS × R2`.

Nœud Custom (CMOT Float 3, même include), entrées `N`, `R1`, `D1`, `G1`, `R2`, `D2`, `G2`, `BaseBlend`, `JunctionStrength`, `Softness`, `GroundRejection`, `DebugShowMask` :

```hlsl
float Mask;
float3 R = DwamNormalTransferHybrid(normalize(N), R1, D1, G1, R2, D2, G2, BaseBlend, JunctionStrength, Softness, float3(0, 0, 1), GroundRejection, Mask);
return lerp(R, Mask.xxx, DebugShowMask);
```

Valeurs de départ : R1 = 5, R2 = 40, BaseBlend = 0.3, JunctionStrength = 1, Softness = 0.5, GroundRejection = 0.8.

## 2. Variante « analytique » : le snake (et plus tard les os)

Exacte, sans GDF : la normale est celle de l'**union lisse** des sphères voisines.

### Données envoyées par `Snake Toy` (Custom Primitive Data)

| Floats | Contenu |
|---|---|
| 0–3 | voisine précédente : offset X, Y, Z (relatif au centre de la sphère), rayon (0 = aucune) |
| 4–7 | voisine suivante : offset X, Y, Z, rayon |
| 8–11 | rayon propre, lissage K (cm), 0, 0 |

### Nœuds à poser

| Nœud | Réglage |
|---|---|
| `Prev` | **Vector Parameter**, *Use Custom Primitive Data* coché, *Primitive Data Index* = **0** |
| `Next` | Vector Parameter, Custom Primitive Data, index **4** |
| `Self` | Vector Parameter, Custom Primitive Data, index **8** |
| `PRel` | `Absolute World Position` − `Object Position` (position relative au centre : précise loin de l'origine) |

### Nœud Custom « DwamSnakeNormal »

- *Output Type* : **CMOT Float 3**
- *Include File Paths* : `/Plugin/DwamLighting/NormalTransfer.ush`
- Entrées (Float3 / Float1 selon le cas) : `PRel`, `PrevPos` (RGB de Prev), `PrevR` (A de Prev), `NextPos` (RGB de Next), `NextR` (A de Next), `SelfR` (R de Self), `K` (G de Self)
- Code :

```hlsl
return DwamChainNormal(PRel, SelfR, float4(PrevPos, PrevR), float4(NextPos, NextR), K);
```

Sortie = normale monde, même branchement que plus haut. Assigner le matériau dans `Material` (catégorie *Rendu*) du Snake Toy.

## 3. Le jouet Snake Toy

*Place Actors* → chercher **Snake Toy**.

- En éditeur, le corps est posé en ligne derrière l'acteur (aperçu du matériau).
- En jeu, la tête se pose sur le sol sous l'acteur puis **rampe** : elle grimpe sur un mur rencontré, contourne les arêtes, retombe si elle perd le sol, et erre avec un bruit de direction. Le corps suit son chemin.
- Les traces sont **complexes** (géométrie visuelle), sur le canal `Trace Channel` (Visibility par défaut).

Réglages :
- *Snake* : `Num Segments`, `Head Radius`, `Tail Radius Ratio`, `Spacing Ratio` (< 1 = sphères qui se chevauchent), `Speed`, `Wander Strength`, `Wander Frequency` ;
- *Rendu* : `Blend K` (largeur du raccord lisse, envoyée au matériau), `Material` ;
- *Debug* : `Draw Debug` (traces, flèche avant jaune, normale cyan).

Sans matériau dédié, le snake marche avec le matériau par défaut : on voit les sphères, sans le lissage.

## 4. Plus tard : personnages en pièces rigides

`DwamMeshNormalWithCapsules(PRel, N, CapA0, CapA1, RadiusA, CapB0, CapB1, RadiusB, K)` fusionne la normale du mesh avec deux capsules voisines (os adjacents). Il manque un composant qui remplisse les Custom Primitive Data de chaque pièce depuis le squelette : à faire avec l'éditeur de personnages.
