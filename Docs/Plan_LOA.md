# Plan — LOA : niveau d'abstraction (Level of Abstraction)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Graph]], [[Plan_KitbashTool]], [[Plan_DreamPlaces]], [[Plan_CityTool]]

> Statut : 🔧 **v1 codée** (2026-10-05) dans le plugin `LOA` de l'auteur : élément LOA, présence en cascade, 2 tests ; appliqué à `LayoutKit` (curseur LOA, 1 test de la maison) ; voir [[Guide_LayoutKit]]. Pas encore vu dans l'éditeur.

## L'existant

`Plugins/LOA` existe déjà (antérieur à la migration en 5.8) : un plugin vide, avec pour description, de la main de l'auteur :

> « Level of Abstraction. Topology Expander pour faire des objets "abstraits", comme passer d'une map de JRPG à un niveau, jusqu'à quelque chose d'hyper réaliste. »

## Ce que l'auteur précise (2026-10-05)

- Le curseur dont il parlait n'est **pas une échelle** (micro / macro) : c'est le **LOA**.
- **Le LOA le plus grand** pour une maison, c'est « genre Pokémon » : une maison réduite à son idée.
- **Le LOA le plus petit** : la maison en plusieurs parties, une terrasse, les fenêtres, une grange collée à l'arrière…

## Lecture (Claude)

- **Le LOA dit combien du graphe est développé.** Au LOA maximal, un seul nœud (la maison, comme sur une carte de JRPG). En baissant le LOA, les nœuds se **développent** en sous-graphes : la maison en corps, entrée, grange ; puis les fenêtres, la terrasse ; puis les détails. C'est le « Topology Expander » de la description.
- **C'est le principe n° 3 vu de l'intérieur** (plusieurs échelles d'un même lieu) : le plan du métro et la carte détaillée sont deux LOA du même lieu. Et c'est valable à toutes les échelles : le monde (la carte de JRPG), la ville, le bâtiment.
- **Ce qui ne disparaît jamais** : ce qui fait l'identité. Une maison Pokémon a toujours sa porte et son toit. Le cœur du graphe reste, les feuilles apparaissent avec le détail (comme pour les lieux rêvés).
- **L'analogie UI** : le *responsive design*. Une page web change de mise en page selon la largeur de l'écran, avec des **points de rupture** ; ici, le bâtiment change de structure selon le LOA. Les rangées de fenêtres en profitent : peu de fenêtres en LOA élevé, la rangée complète en LOA bas.
- **Quand un élément disparaît** en montant le LOA, deux comportements possibles :
  - **il disparaît** (une fenêtre, une rambarde) ;
  - **il est absorbé par son parent** (la grange se fond dans un volume plus grand) : la silhouette générale est gardée. C'est l'**agrégation** de la généralisation cartographique.
- **Le lien avec Dormans** ([[Plan_DreamPlaces]]) : développer un nœud en sous-graphe, c'est exactement une **grammaire de graphes**. Le « Topology Expander » peut être un jeu de **règles de réécriture** : « une maison = un corps + une entrée + (en LOA bas) une grange ».

## Première application : LayoutKit

Proposition pour la v1 de [[Plan_KitbashTool]] :
- un **curseur LOA** sur l'acteur (des niveaux entiers : 0 = le plus détaillé, plus grand = plus abstrait) ;
- chaque élément (boîte, pièce) dit **jusqu'à quel LOA il existe**, et s'il **disparaît** ou **est absorbé** au-delà ;
- les rangées choisissent leur nombre d'éléments selon le LOA.

Plus tard, les règles de développement (la grammaire) dans le plugin `LOA`, au-dessus de `GraphKit`.

## Décisions (2026-10-05)

- **Entier ou continu : les deux** (décision de Claude, l'auteur lui fait confiance). L'auteur : « en pratique, il faut de l'entier », mais les arbres façon *Far Cry* poussent, et « ce serait cool de le voir se déplier ».
  - **La partie entière fait la structure** : chaque élément dit **jusqu'à quel niveau entier** il existe (écrit à la main).
  - **La partie décimale fait le dépliage** : entre deux niveaux, les éléments du niveau suivant **poussent** (ou se fondent dans leur parent). Le même mécanisme servira à la croissance des arbres.
- **Le code va dans le plugin `LOA`** existant (« carrément, c'est l'occasion de le faire sans limite »). Il dépend de `GraphKit` ; `LayoutKit` s'en sert.
- **L'échelle reste**, en plus du LOA : le LOA dit **combien de détails**, l'échelle (le gizmo, fixe / étirable) dit **quelle taille**.

## v1 (codée)

- **Correction apportée par les tests** : une boîte **entièrement absorbée ne disparaît pas**, elle fait partie du volume de son parent ; seuls ses propres enfants s'effacent. (La première version la faisait disparaître à LOA 4.)

- Plugin `LOA` : un élément LOA (`Visible jusqu'au niveau`, `S'efface` ou `Absorbé par le parent`) et le calcul de la **présence** de chaque nœud (0 à 1) pour une valeur de LOA, en cascade depuis la racine (un enfant ne peut pas être plus présent que son parent). Tests automatiques.
- `LayoutKit` : un curseur **LOA** sur l'acteur ; une boîte qui s'efface **rapetisse depuis son point d'attache** ; une boîte absorbée **prend la section de son parent** (même largeur de côté, même hauteur, coins vifs), ce qui fond les deux volumes en un bloc.
- L'exemple : corps (toujours), entrée (toujours, c'est l'identité : une maison Pokémon a sa porte), terrasse (jusqu'au niveau 0, s'efface), grange à l'arrière (jusqu'au niveau 1, absorbée).
