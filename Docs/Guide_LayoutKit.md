# Guide — GraphKit et LayoutKit (v0)

> Accueil : [[00 - Accueil]] · Plans : [[Plan_Graph]], [[Plan_KitbashTool]]

> État (2026-10-05) : `GraphKit`, `LOA` et `LayoutKit` compilent ; **11 tests automatiques réussis** (3 GraphKit, 2 LOA, 6 LayoutKit). **Pas encore vu dans l'éditeur.**

## GraphKit : le socle

Un graphe sans position : des **nœuds** (un nom unique, des tags, une charge utile libre) et des **arêtes** (départ, arrivée, **distance**, tags, charge utile), orienté ou non.

- C'est une **struct** (`Graph Kit Graph`) : on la met en variable d'un acteur, dans un Data Asset, n'importe où.
- En Blueprint, la bibliothèque **GraphKit** : `Add Node`, `Remove Node`, `Rename Node` (met à jour les arêtes), `Add Edge`, `Remove Edges`, `Find Node`, `Get Neighbors` (avec la distance), `Get Nodes With Tag`, `Validate` (liste les erreurs d'écriture).
- **Charge utile** : dans le panneau Détails, choisir le type de struct dans la liste déroulante du champ *Payload*, puis remplir ses champs.

## LayoutKit : un bâtiment comme en UI

### Essayer en une minute

1. *Place Actors* → chercher **Layout Kit Actor**, le poser.
2. Il contient déjà l'exemple : un **corps** (10 × 8 × 6 m, étirable, avant légèrement arrondi), une **entrée** (3 × 2,5 × 3,5 m, fixe, toujours là), une **terrasse** basse sur le côté droit et une **grange** collée à l'arrière, au fond arrondi.
3. **Mettre l'acteur à l'échelle** avec le gizmo : le corps grandit, **l'entrée garde sa taille humaine** et reste collée à l'avant. Le mesh n'est jamais étiré.
4. Bouton **Reset To Example** pour revenir à l'exemple.
5. **Faire glisser le curseur LOA** de 0 à 4 : vers 0,5 la terrasse se replie, à 1 elle a disparu ; entre 1 et 2 la grange s'élargit et monte pour se fondre dans le corps ; au-delà, il reste « la maison Pokémon » : un bloc et son entrée. Entre deux niveaux entiers, on voit la maison se (dé)plier.

### Les réglages

Dans *Layout* (le graphe, orienté) :
- **Un nœud = une boîte.** Charge utile **Layout Kit Box** :
  - *Width* (largeur), *Depth* (profondeur, l'avant est en +X), *Height* : chacune **Fixe** ou **Étirable** ;
  - les 4 rayons de coin (*Front Left*, *Front Right*, *Back Right*, *Back Left*), comme le `border-radius` du CSS ; 0 = coin vif. Trop grands, ils sont réduits automatiquement ;
  - *Corner Segments* : la finesse des arrondis.
- **Une arête = une attache**, du parent (*From*) vers l'enfant (*To*). Charge utile **Layout Kit Attach** :
  - *Side* : avant, arrière, gauche, droite ;
  - *Alignment* : 0 = début du côté, 0,5 = centre, 1 = fin (l'enfant reste dans le côté) ;
  - *Overlap* : de combien l'enfant entre dans le parent. Il dépasse donc de sa profondeur moins ce recouvrement.
- L'enfant est **tourné vers l'extérieur** : une boîte attachée à droite a son avant qui regarde à droite. Un enfant peut avoir ses propres enfants.

**LOA** (niveau d'abstraction) : dans chaque boîte, *LOA* → *Visible Up To* (jusqu'à quel niveau elle existe) et *Collapse* (**S'efface** : elle rapetisse depuis son point d'attache ; **Absorbé par le parent** : elle prend la largeur du côté et la hauteur de son parent, coins vifs, et fait corps avec lui). Les enfants d'une boîte repliée se replient avec elle. En Blueprint, **Set LOA** change le niveau et reconstruit (pour l'animer en jeu). Voir [[Plan_LOA]].

*Root Box* : la boîte de départ, posée au centre de l'acteur. *Material*, *Generate Collision*.

*Problems* (en lecture seule) liste ce qui ne va pas : racine inconnue, nœud sans boîte, nœud non relié, arête vers un nœud absent.

### À savoir (v0)

- **Les volumes se recouvrent** au lieu d'être fusionnés : l'entrée traverse un peu le mur du corps, la partie cachée ne se voit pas.
- Les boîtes sont posées **au sol** (pas encore d'étages ni de hauteur de départ).
- L'écriture du graphe se fait dans le panneau Détails (listes de nœuds et d'arêtes) : pratique pour deux ou trois boîtes, pénible au-delà. Un vrai outil d'écriture est prévu ([[Plan_Graph]]).

## Ce qui vient ensuite (proposé)

Étages et hauteur de départ ; le **9-slice** (extrémités fixes, milieu étiré) ; des pièces de kit (portes, fenêtres) placées comme des enfants ; les tableaux (une rangée de fenêtres) ; la fusion des volumes si elle devient utile.
