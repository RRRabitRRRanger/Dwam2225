# Plan — Composer vite un mesh à partir de formes simples (BP_Edition)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_CityTool]], [[Plan_NPC]], [[Plan_Loot]]
> Source : le doc de l'auteur « Idées d'outils Unreal » (Claude Docs, 2026-10-05).

> Statut : 🔧 **v0 codée** (2026-10-05) : plugin `LayoutKit` (boîte aux coins arrondis par coin, entrée qui dépasse, fixe / étirable), 5 tests automatiques réussis ; guide [[Guide_LayoutKit]]. **Outil Unreal, pas strictement DWAM.** Pas encore vu dans l'éditeur.

## L'idée (auteur)

Construire très vite un « mesh » à partir d'une bibliothèque de formes simples : cubes, cônes, etc.

**Workflow avec BP_Edition** :
1. Construire l'objet avec des meshes directement dans la scène (par exemple une barrière).
2. Ajouter un « objet » pour définir l'origine.
3. Dans BP_Edition, glisser-déposer chaque mesh de la composition, puis sauvegarder.

**Ce n'est pas assez paramétrique.** Pour le rendre paramétrique, s'inspirer des **mises en page d'UI** : marges, espacement…
- Si je mets l'objet à l'échelle, la composition s'adapte.
- Ensuite, je peux en faire un tableau (array).
- Pareil pour des fenêtres.

**Stockage** : un Data Asset, avec des listes horizontales, verticales et en profondeur, ou des tableaux.

**Exemples** : des tas de poutres ou de terre paramétriques, des rayons.

## Remarques (Claude)

- **Le modèle des boîtes d'UMG en 3D** : des conteneurs (horizontal, vertical, profondeur, grille) avec marges, espacement, alignement, et des règles de taille (« remplir », « taille propre », proportion). Une composition est un **arbre de conteneurs**, stocké dans le Data Asset. Changer l'échelle recalcule la mise en page au lieu d'étirer les pièces.
- **Le même outil sert ailleurs** :
  - **les créatures en pièces rigides** de [[Plan_NPC]] : assembler des formes simples sur un squelette ;
  - **les armes en pièces** de [[Plan_Loot]] : lame, garde, poignée, pommeau.
  Un seul outil pour les trois serait un vrai gain.
- **Le résultat** : des instances (rapide, modifiable), ou un mesh fusionné par Geometry Script (déjà utilisé dans `CitySplineTools`).

## Bâtiments : kit + règles de mise en page + graphe (auteur, 2026-10-05)

L'auteur : « on peut fabriquer les bâtiments avec du kit », avec des règles **façon mise en page d'UI** (marges, espacement). **Plus le graphe**, ça veut dire pouvoir avoir **Fort-Dragon** (*Dragonsreach*, le palais de Blancherive) en plus petit que dans *Skyrim*, comme dans *Skyrim*, ou à l'échelle de *The Witcher 3*.

### Lecture (Claude)

- **L'échelle devient un réglage.** Le graphe garde l'identité du bâtiment (ses pièces, leurs liens, leurs rôles : la grande salle, le trône, les escaliers vers la terrasse) ; la mise en page produit la géométrie **à l'échelle choisie**. C'est le principe n° 3 (plusieurs échelles d'un même lieu) appliqué aux bâtiments.
- **Comme en UI, des éléments fixes et des éléments souples.** Une porte, une marche, une rambarde gardent une **taille humaine** quelle que soit l'échelle (en UI : un bouton de taille fixe). La grande salle, les couloirs, les cours **s'étirent** (en UI : « remplir l'espace »). Sans cette distinction, agrandir un bâtiment donnerait des portes de géant.
- **Une seule structure pour la ville et le bâtiment** : un bâtiment est un nœud du graphe de la ville, qui contient son propre graphe (ses pièces). C'est le graphe hiérarchique ([[Plan_DreamPlaces]]).

## Version 0 : `LayoutKit`, un bâtiment comme en UI (2026-10-05)

> Plan écrit puis code lancé avec l'accord de l'auteur (« fais le plan, puis au travail là-dessus »).

**Premier objet, choisi par l'auteur** : un bâtiment en forme de **boîte**, aux **coins plus ou moins arrondis** (ou pas du tout), **chaque coin pouvant être différent**, avec peut-être une **entrée qui dépasse**.

### Le principe

- **Le bâtiment est un graphe** ([[Plan_Graph]], `GraphKit`) : chaque **nœud** est une boîte (corps principal, entrée…), chaque **arête** est une **attache** du parent vers l'enfant (sur quel côté, où, de combien elle dépasse). C'est l'arbre d'une interface : une boîte qui contient ou porte d'autres boîtes.
- **Les règles viennent de l'UI** :
  - **coins arrondis par coin**, comme le `border-radius` du CSS (4 rayons, 0 = coin vif) ;
  - **tailles fixes ou souples** : chaque dimension est **Fixe** (taille humaine, ne bouge pas) ou **Étirable** (suit l'échelle) ;
  - **attache** sur un côté, avec un **alignement** le long du côté (0 = début, 0,5 = centre, 1 = fin), un **recouvrement** (de combien il entre dans le parent, pour que les volumes se touchent) ; l'**avancée** en découle : sa profondeur moins le recouvrement.
- **Mettre l'acteur à l'échelle ne déforme rien** : l'échelle de l'acteur est lue comme une **taille demandée**. Les dimensions étirables grandissent, les fixes gardent leur taille (l'entrée reste à taille humaine), et le mesh n'est jamais étiré. C'est la démonstration du principe « éléments fixes et souples ».

### Dans la v0

- **Plugin `LayoutKit`** (dépend de `GraphKit`), un acteur **`ALayoutKitActor`** : un graphe de boîtes, le nom de la boîte racine, un matériau, la collision ; il reconstruit son mesh (Dynamic Mesh) à chaque modification dans l'éditeur.
- **Une boîte** : largeur, profondeur, hauteur (chacune fixe ou étirable), 4 rayons de coin, finesse des arrondis.
- **Une attache** : côté (avant, arrière, gauche, droite), alignement, recouvrement (l'avancée = la profondeur de l'enfant moins le recouvrement).
- **Le mesh** : chaque boîte est un prisme fermé (murs, sol, toit), aux arrondis lissés et aux coins vifs nets. **Les volumes se recouvrent** au lieu d'être fusionnés : simple et solide. Une vraie fusion (booléen) viendra si elle devient nécessaire.
- **Des tests automatiques** : contour d'une boîte (rayons limités à ce qui tient, contour convexe), placement d'une entrée, effet de l'échelle sur le fixe et l'étirable, prismes fermés.

### Hors v0

Les étages, les fenêtres en tableau, le 9-slice (extrémités fixes, milieu étiré), les formes non rectangulaires, la fusion des volumes, un vrai outil d'écriture du graphe.

## Version 1 : ce que l'auteur veut placer (2026-10-05, proposition en attente de validation)

**Demandes de l'auteur** : des **portes**, des **fenêtres**, des **balcons**, des **toits** (peut-être de différents types), une **tour**, peut-être un garage (« bon, peut-être pas »), mais des **fermes** : les animaux souvent **en dessous**, un côté **grange**. Et **la même maison à différentes échelles** (le « mini Skyrim » ou *The Witcher 3*).

### Comment ça entre dans le système (Claude)

| Demande | Ce qu'il faut ajouter |
|---|---|
| **Étages, animaux en dessous** | Une attache **sur le dessus** (côté « Haut ») : une boîte posée sur une autre. Le rez-de-chaussée est l'étable, l'étage l'habitation. Les **tags** des nœuds disent le rôle (étable, habitation, grange) |
| **Balcon** | Une attache **en hauteur** (une élévation) sur un côté, une boîte peu profonde, plus une rambarde (pièce de kit) |
| **Toits** | Un toit par boîte, au choix : **plat**, **appentis** (un pan), **deux pans**, **quatre pans**, **cône** (pour une tour), avec un **débord**. Le toit suit les tailles fixes ou étirables comme le reste |
| **Tour** | Déjà possible : une boîte aux 4 coins au rayon maximal devient un **cylindre**. Il lui manque le toit en cône |
| **Portes, fenêtres** | Des **pièces de kit** (meshes) attachées à un côté, à une hauteur et un alignement donnés, **toujours de taille fixe** (taille humaine). D'abord posées sur le mur (un cadre) ; un vrai trou dans le mur demande un booléen, plus tard |
| **Rangées de fenêtres** | Une règle de **répétition**, comme une rangée d'UI : un espacement et des marges ; le nombre de fenêtres s'adapte à la longueur du mur |
| **Grange, ferme** | Une combinaison des précédents : grand volume, toit à deux pans, grande porte |
| **La même maison à différentes échelles** | Des **profils d'échelle** nommés (« compact », façon *Skyrim*, façon *The Witcher 3*) : un multiplicateur pour l'étirable, le fixe ne bouge jamais. C'est la **compression d'échelle** des jeux en monde ouvert |

### Décisions de l'auteur (2026-10-05)

- **Rangées** : comme les *Vertical / Horizontal Box* d'UI. En 3D, il faudra peut-être quelque chose de moins « fermé », mais c'est bien pour commencer.
- **Portes et fenêtres** : importantes au final, **et à différencier**. Pour l'instant, des **formes simples** (cube, sphère, etc.) suffisent.
- **Les toits complexes plus tard** : c'est un outil en soi.
- **L'échelle est importante** : pouvoir non seulement faire un bâtiment, mais **glisser du micro au macro**. Un curseur, pas seulement des profils nommés.

### Correction de l'auteur (2026-10-05) : ce n'est pas une échelle, c'est le LOA

Le curseur s'appelle **LOA** (*Level of Abstraction*) : du plus abstrait (« genre Pokémon ») au plus détaillé (la maison en plusieurs parties, une terrasse, les fenêtres, une grange collée à l'arrière). Voir [[Plan_LOA]] ; le plugin `LOA` existait déjà, vide, avec cette idée. La lecture « micro / macro » ci-dessous est **remplacée**.

### ~~Lecture du « micro au macro » (Claude)~~ (remplacée par le LOA)

- **Un curseur d'échelle** sur l'acteur : à un bout le micro (très compressé, le « mini Skyrim »), à l'autre le macro (ample, façon *Witcher 3*). L'étirable suit le curseur, le fixe (portes, marches, fenêtres) ne bouge jamais.
- **Interpolation géométrique** entre les deux bouts (comme un zoom), pour que le milieu du curseur paraisse vraiment « à mi-chemin ».
- **Comme en UI, plus de place = plus d'éléments, pas des éléments plus gros** : en macro, un mur plus long reçoit **plus de fenêtres** (la répétition s'adapte), pas des fenêtres géantes. C'est ce qui rend le curseur crédible.

### Ordre révisé

1. ~~Le curseur micro → macro~~ → **le curseur LOA** ([[Plan_LOA]]).
2. **Élévation et attache sur le dessus** (étages, balcon, étable en dessous).
3. **Pièces simples** (cube, cylindre, sphère, cône) de taille fixe, attachées à un mur à une hauteur donnée, avec un rôle (porte, fenêtre) pour les différencier.
4. **Répétition** (rangées façon Horizontal / Vertical Box).
5. Plus tard : les toits complexes (un outil en soi).

### Ordre proposé à l'origine

1. **Élévation et attache « Haut »** : étages, balcons, étable en dessous. Le plus structurant.
2. **Toits** : plat, appentis, deux pans, quatre pans, cône.
3. **Pièces de kit** : portes et fenêtres de taille fixe.
4. **Répétition** : rangées de fenêtres.
5. **Profils d'échelle.**

### Questions

- Dans quel ordre, si celui-ci ne convient pas ?
- Les portes et fenêtres : as-tu des meshes, ou faut-il en générer de simples (cadres) pour commencer ?
- Les toits sur des coins arrondis : un toit à deux pans sur une boîte aux coins arrondis n'a pas de forme évidente. On limite les toits à pans aux coins vifs, ou tu as une idée ?

## Questions ouvertes

- Une composition doit-elle rester modifiable dans la scène, ou devenir un asset figé ?
- Faut-il des pièces paramétriques elles-mêmes (une poutre dont on règle la longueur), ou seulement des pièces fixes disposées ?

## Ombres et distance field : pièces déjà cuites (idée de l'auteur, 2026-10-12)

Constat : le mesh de LayoutKit est dynamique, donc absent du Global Distance Field ; il n'a ni ombre en ray-march ni transfert de normales. L'option « distance field » des meshes dynamiques est dépréciée et sans effet en 5.8 (`BaseDynamicMeshComponent.h`).

**Idée de l'auteur** : assembler la forme avec des **static meshes déjà cuits** (cube, quart de cylindre, « des trucs du genre »), qui ont chacun leur mesh distance field, pour obtenir une approximation du SDF de l'ensemble. Décision : « on garde comme ça pour l'instant », rien à coder.

Lecture de Claude :

- une boîte aux coins arrondis se décompose en un cube (le cœur), des quarts de cylindre (les arêtes arrondies) et, si les coins sont arrondis dans les trois axes, des huitièmes de sphère ;
- les pièces étant des assets cuits dans l'éditeur, l'assemblage fonctionnerait aussi pour des bâtiments générés pendant le jeu, là où une cuisson à la volée n'est pas possible (calcul des mesh distance fields réservé à l'éditeur : `À COMPLÉTER`, pas revérifié dans le source) ;
- à vérifier le moment venu : la tenue du distance field sous une échelle non uniforme (pièces étirées).

Autres pistes notées le même jour, non retenues pour l'instant : un bouton « Bake to Static Mesh » (un asset par forme arrêtée) ; un SDF analytique évalué dans le shader à partir d'une liste de boîtes envoyée comme les lumières ([[Plan_Lights]]).
