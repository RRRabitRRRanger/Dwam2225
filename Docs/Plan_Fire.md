# Plan — Feu qui se propage

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Water]], [[Plan_PatchTerrain]], [[Plan_WorldState]]

> Statut : 💭 **idée**, en discussion avec l'auteur (2026-10-04). Rien n'est décidé.

## Intention

Un feu qui peut s'étendre à l'échelle d'une prairie, dans l'esprit du moteur « chimique » de *Breath of the Wild*.

## Inspirations

- **Breath of the Wild** (conférence GDC 2017) : un *chemistry engine* à trois règles :
  - un élément change l'état d'un matériau (le feu brûle le bois) ;
  - un élément change un autre élément (l'eau éteint le feu) ;
  - un matériau ne change pas un autre matériau.

  Le reste en découle : le vent pousse le feu, la pluie l'éteint, le métal attire la foudre.
- **Premiers Far Cry** (cités par l'auteur) : le monde guérit, même une branche coupée.

## Idée de l'auteur : des nodes (2026-10-04)

Certains objets font apparaître un **node** et ses voisins. Le graphe se construit et se développe à partir de là, et ce qui est proche d'un node peut prendre feu. Pour une prairie : **l'herbe est regroupée par plaques**, pas brin par brin (validé par l'auteur).

Analyse (Claude) : c'est une bonne approche.
- Une grille est un graphe dont les voisins sont implicites ; les nodes sont le même principe avec des voisins explicites. On peut combiner les deux.
- **Graphe construit seulement autour du feu** : un objet qui s'enflamme crée son node, cherche le combustible proche, crée ces voisins. Rien n'est précalculé pour toute la carte, on ne paie que là où le feu passe (même philosophie que Watershed).
- **Marche en 3D** : arbre, maison, caverne, pas seulement le sol.
- Un node peut être une plaque d'herbe (1–2 m), un arbre entier, une caisse, un ennemi.
- À prévoir : un **index spatial** pour trouver le combustible proche (végétation instanciée, objets).

## Contenu d'un node (proposition)

- Type de combustible (herbe, broussaille, bois, tissu, huile…), quantité restante.
- **Humidité** : pluie, flaques et lacs de Watershed.
- Température, et état : intact, en feu, brûlé, puis repousse.

**Propagation** : la chaleur passe d'un node à ses voisins, pondérée par la distance, le **vent**, la **pente** (un feu monte plus vite qu'il ne descend) et l'humidité.

## Liens avec les autres systèmes

- **Watershed** : l'eau mouille, la pluie éteint, un lac fait coupe-feu.
- **PatchTerrain** : les cellules Voronoi donnent le type de sol, donc de combustible.
- **Éclairage maison** : un grand feu ferait des centaines de lumières. Il faut **une lumière par foyer** (regroupement), d'où l'intérêt du culling de lumières prévu.
- **Rendu** : terre brûlée (L d'OKLab plus bas), braises émissives, fumée et flammes en Niagara.

## Pistes de gameplay

Flèche enflammée dans les herbes hautes, huile renversée, fumée qui bloque la vue, statut « en feu » qui passe d'un ennemi à l'autre, lianes brûlées qui révèlent un passage. Plus tard, généraliser aux autres éléments (eau, froid, foudre) selon les trois règles de BOTW.

## Questions ouvertes

- D'où viennent les nodes de combustible : végétation instanciée, PCG, composants posés à la main ?
- Repousse : durée, et ce qui persiste (voir [[Plan_WorldState]]).
