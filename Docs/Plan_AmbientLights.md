# Plan — Lumières d'ambiance : des surfaces qui émettent leur rebond

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Focus]] · [[Plan_LightCulling]] · [[Guide_OKLab]] · [[Plan_GpuTraces]] · [[Plan_NormalTransfer]]

> Statut : idée de l'auteur (2026-10-11). **Décision (auteur, 2026-10-11) : la méthode retenue est celle des surfels** (plans émissifs traités comme un type de lumière), après comparaison avec les points virtuels, la grille de sondes et le splatting. Le découpage en étapes ci-dessous reste une proposition de Claude, pas validée. Rien de codé.

## L'idée (auteur)

- Un **type de lumière pensé pour l'ambiance**, à côté des point, spot et rect : de vraies lumières, pas un effet à part.
- Pas des points : des **cubes**, c'est-à-dire des **plans émissifs**. « Je crois que c'est ce que ça devrait être. »
- Le but, c'est la couleur qui déteint : **un citron dans une serviette met du jaune dans la serviette** ; pareil au pied d'un mur.
- **Pas en screen space** : l'ambiance ne doit pas dépendre de ce qui est à l'écran.
- Répartition en **quadtree** (gros carrés loin, petits carrés près), comme les tuiles de [[Plan_Water]].
- Rêve : les placer **sur GPU, en particules**, comme des splats.
- Budget des lumières directes : **32, voire 64**, avec de la marge.

## Comment ça s'appelle

L'idée existe sous plusieurs noms ; ce sont les mots-clés pour creuser.

| Nom | Ce que c'est | Référence |
|---|---|---|
| **Surfel** (surface element) | un petit disque orienté posé sur une surface, qui émet la lumière qu'il reçoit, teintée par sa couleur | Bunnell, *Dynamic Ambient Occlusion and Indirect Lighting*, GPU Gems 2 (2005) ; EA, *Global Illumination Based on Surfels* (SIGGRAPH 2021) |
| **Cards** | des plans autour d'un objet qui stockent sa couleur éclairée : exactement « un cube de plans émissifs » | le surface cache de Lumen |
| **Splatting de l'indirect** | chaque source de rebond est dessinée comme une tache qui éclaire ce qu'elle recouvre | Dachsbacher & Stamminger, *Splatting Indirect Illumination* (2006) |
| **Virtual point lights** | la version « points » de la même idée | Keller, *Instant Radiosity* (1997) ; Walter, *Lightcuts* (2005) pour la hiérarchie |

## Pourquoi un plan vaut mieux qu'un point (lecture de Claude)

- **Un plan n'émet que d'un côté.** Le sol éclaire le pied du mur, pas la cave en dessous : une bonne partie des fuites de lumière des points disparaît sans calcul d'ombre.
- **Pas de singularité.** L'éclairement d'un disque d'aire A, vu à la distance d, reste borné quand d tend vers 0 (forme de Bunnell) :

```latex
E = C \cdot \frac{A \cos\theta_e \cos\theta_r}{\pi d^2 + A}
```

  avec C la couleur émise, θe l'angle à la normale de l'émetteur, θr l'angle à la normale du receveur.
- **Le citron se culle très bien** : petit émetteur, petite portée, peu de tuiles.

## Méthodes comparées (2026-10-11)

| Méthode | Pour | Contre | Verdict |
|---|---|---|---|
| Points virtuels (VPL) | type point existant, rien à inventer | fuites à travers les murs, point brûlé de près, il en faut beaucoup | écartée |
| **Plans émissifs (surfels)** | même pipeline (packing, culling, shading OKLab), peu de fuites grâce à l'orientation, pas de point brûlé, réglable un par un | un type à ajouter, grandes surfaces chères sans quadtree, pas d'ombre | **retenue** |
| Grille de sondes (clipmap) | coût par pixel fixe, très lisse | système à part, détail limité par la maille (le citron disparaît), fuites à travers les murs fins | complément possible plus tard, pour l'ambiance large |
| Splatting | coût proportionnel à la surface couverte | suppose un rendu différé avec G-buffer ; le culling par tuiles fait déjà ce travail | écartée |

## Étapes proposées (Claude, à valider)

| Étape | Livrable | Validation |
|---|---|---|
| 1. Type « surfel » dans le shader | fonction d'éclairement d'un disque orienté dans `Lighting.ush` (forme de Bunnell, portée fenêtrée, un seul côté) | un surfel réglé à la main dans le matériau éclaire un mur comme attendu ; pas de point brûlé de près |
| 2. Packing | le type surfel dans la texture des lumières : position, normale, aire, couleur, portée | voir « Données d'un émetteur » ci-dessous : texture à part, 2 pixels par émetteur, sans type |
| 3. Émetteur posé à la main | composant « émetteur d'ambiance » : une boîte sur un acteur, six plans, couleur réglée à la main | le citron dans la serviette ; le pied d'un mur |
| 4. Couleur qui suit l'éclairage | intensité de chaque face = éclairage direct qu'elle reçoit | le citron déteint moins à l'ombre |
| 5. Placement automatique | traces depuis les lumières ([[Plan_GpuTraces]]), un surfel par impact, regroupés en quadtree autour du joueur | une pièce entière sans émetteur posé à la main |
| 6. Tout sur GPU | passe compute qui écrit les surfels dans le buffer des lumières de [[Plan_LightCulling]] | pas de coût CPU par surfel |

### Données d'un émetteur (2026-10-11)

**Auteur** : les lumières directes portent température, teinte, position, ombre, layer et type (de mémoire, disposition exacte **À COMPLÉTER**). Pour l'ambiance, pas besoin du type, « on sait que c'est de l'ambient » : **position, rotation, scale, couleur**.

**Proposition de Claude** : avec position + rotation + scale, la primitive est une **boîte orientée** (le « cube » de l'auteur) : une entrée vaut six plans, et une boîte aplatie vaut un plan seul. Ça tient dans 2 pixels RGBA32F, comme une lumière directe :

| Pixel | R | G | B | A |
|---|---|---|---|---|
| 0 | position X | position Y | position Z | rotation : quaternion compressé en 32 bits (les trois plus petites composantes) |
| 1 | demi-taille X | demi-taille Y | demi-taille Z | couleur émise : RGB HDR compressé en 32 bits (exposant partagé, type RGB9E5) |

- **Portée** : déduite de la taille et de l'intensité, pas stockée.
- **Pas d'auto-éclairage** : chaque face n'émet que vers l'extérieur ; un point à l'intérieur de la boîte est derrière les six faces et ne reçoit rien. Une boîte qui enveloppe le citron n'éclaire donc pas le citron.
- **Coût** : au plus trois faces visibles depuis un point, donc trois évaluations de plan par émetteur.
- **Type** : inutile tant que les émetteurs ont leur propre texture. Si un jour ils partagent la liste triée du culling avec les lumières directes, il faudra un bit pour les distinguer (à prendre sur la rotation ou la couleur).
- **À valider** : précision du quaternion et de la couleur en 32 bits ; aller-retour `asfloat` / `asuint` (même précaution que dans [[Plan_LightCulling]]).

Les étapes 1 à 3 ne dépendent pas du culling : avec quelques surfels, la boucle brute-force suffit. Les étapes 5 et 6 demandent le culling en place.

## Est-ce que ça va marcher ? (doute de l'auteur, 2026-10-11)

L'auteur : « ça ne sera pas une implémentation bas niveau », et « il faudra régulièrement les jeter, les confettis ». Lecture de Claude, a priori, rien de mesuré :

- **Émetteurs posés à la main (étapes 1 à 3) : oui.** C'est la même boucle que les lumières directes, avec un calcul plus léger (pas de ray-march d'ombre). Rien n'y demande un accès au moteur.
- **Jeter les confettis (étape 5) : c'est la partie risquée.**
  - *Pas à chaque frame* : le rebond change lentement. On relance quelques traces par frame, à tour de rôle ; scène et lumières immobiles, on ne relance rien.
  - *Stabilité* : un confetti qui change de place fait scintiller l'ambiance. Un seul émetteur par case du quadtree, à place fixe dans la case, avec un fondu à l'apparition et à la disparition.
  - *La couleur à l'impact* : c'est la vraie limite du « pas bas niveau ». Une trace donne l'acteur et le matériau touchés, pas leur couleur. Il faut une « couleur de rebond » par matériau ou par acteur (réglée à la main ou moyennée une fois), ou bien une capture de scène depuis la lumière (reflective shadow map), plus chère.
- **Ce que ça ne sera pas** : du Lumen. Un rebond grossier, stylisé, sans ombre ; ~~cohérent avec un rendu non photoréaliste~~ (faux : le rendu n'est pas non photoréaliste, correction de l'auteur du 2026-10-17 ; voir [[Plan_Forward]]).
- **Conséquence** : juger le résultat à l'étape 3 (à la main) avant d'investir dans les confettis.

## Un éclairagiste automatique, pas une simulation (auteur, 2026-10-11)

**L'auteur** : les pros qui posaient ces lumières à la main avaient une direction, de l'authoring, un choix. On n'est donc **pas obligé de couvrir toute la scène** : seulement les endroits qu'on veut **détailler** et ceux qu'on veut **déboucher**. Ici, l'authoring serait **en temps réel, automatique**.

**Lecture de Claude** : le problème change de nature. Il ne s'agit plus de simuler le rebond partout, mais d'automatiser les choix d'un éclairagiste. C'est le principe fondateur n° 4 (« on ne simule que ce que le joueur peut sentir ») appliqué à la lumière.

**Proposition (Claude, à discuter)** : un « directeur » avec un budget fixe d'émetteurs.

- **Budget fixe** (par exemple 16 à 32 émetteurs actifs), quel que soit le lieu : le coût par pixel est borné et le dimensionnement de [[Plan_LightCulling]] ne bouge pas.
- **Deux familles de candidats** :
  - *détailler* : les objets colorés et éclairés près d'une surface (le citron) ;
  - *déboucher* : les zones d'ombre proches du joueur, à côté d'une surface éclairée (le pied du mur).
- **Une note par candidat** (taille à l'écran, saturation, lumière reçue, proximité d'une ombre) ; on garde les meilleurs.
- **Hystérésis et fondu** : un émetteur retenu le reste un moment, pour que le choix ne clignote pas.
- **Prix à payer** : ce n'est pas physique. Un rebond peut apparaître quand on s'approche ; le fondu doit le rendre invisible.

## Questions ouvertes

- ~~**Qui désigne les candidats** : des objets marqués par l'auteur (un composant ou un tag), des règles entièrement automatiques, ou les deux ?~~ Réponse de l'auteur (2026-10-11) : des points d'intérêt déclarés par component ou user data, triés par score, dans un système de focus à moitié automatique, à moitié authored. Voir [[Plan_Focus]].

- **Budget** : les émetteurs d'ambiance s'ajoutent aux 32–64 lumières directes. Dimensionner le culling pour 128 ou 256 ? (voir [[Plan_LightCulling]])
- **D'où vient la couleur émise** : réglée à la main, moyenne de la texture, ou lue sur le rendu ? Piste de l'auteur (2026-10-11) : la **palette du niveau** généré en PCG la fournit, voir [[Plan_Focus]].
- **Occlusion** : rien, portée courte, ou un test grossier sur le Global Distance Field ?
- **Grandes surfaces** (un sol entier) : un gros émetteur se culle mal ; combien de niveaux de quadtree ?
- **Repli** si le coût par pixel monte : grille de sondes en clipmap (une lecture de texture par pixel).
