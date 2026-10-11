# Plan — Focus : un point d'attention qui guide le regard

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_AmbientLights]] · [[Plan_LightCulling]] · [[Plan_GpuTraces]] · [[Plan_DreamPlaces]] · [[Plan_PostProcess]]

> Statut : **idée** de l'auteur (2026-10-11), notée en discussion. Rien de validé, rien de codé. Les parties « Lecture » et « Proposition » sont de Claude.

## L'idée (auteur)

- Un **focus** : une « sphère » qui se balade dans le monde, **à moitié automatique, à moitié authored**. Exemple : quand on passe dans un volume, ce point se déplace.
- Il **change subtilement** des effets de shaders, de particules, etc.
- Des **points d'intérêt**, déclarés par un component ou des user data, **triés par score**. Le soleil aurait un score.
- Conséquence voulue : même en intérieur, **vers la sortie d'une caverne, le soleil ferait un bounce jusqu'à nous**. Piste de l'auteur : « probablement un Bug algo en GPU » (compris par Claude comme l'algorithme Bug de l'agent de [[Plan_GpuTraces]], pour trouver le chemin de la lumière ; à confirmer).
- Le but : **des choses qui guident subtilement le regard**.

## Lecture de Claude

- **C'est le « directeur » de [[Plan_AmbientLights]], en plus large.** Les points d'intérêt et leur score répondent à la question « qui désigne les candidats » : le focus choisit, les lumières d'ambiance ne sont qu'un de ses clients (avec les shaders et les particules).
- **Les points d'intérêt sont les « repères » de Kevin Lynch** (principe fondateur n° 2, [[Plan_DreamPlaces]]) : ce qui rend un lieu mémorable est aussi ce vers quoi on guide le regard.
- **Guider par la lumière est un métier connu** en level design (la lumière comme appât au bout d'un couloir). Ici, ce serait fait par un système plutôt que posé à la main.

## Proposition (Claude, à discuter)

**Trois briques**

1. **Point d'intérêt** : un component sur un acteur, avec un score de base, un rayon et des tags. Le soleil en a un.
2. **Le focus** : un world subsystem qui collecte les points d'intérêt, calcule leur score du moment (score de base, distance, visibilité, contexte), les trie, et en déduit la position et le rayon de la sphère. Les volumes authored ajoutent un candidat à fort score ou imposent la position. Le déplacement est amorti (ressort), jamais sec.
3. **Les sorties** :
   - position et rayon de la sphère dans une Material Parameter Collection (shaders) et une Niagara Parameter Collection (particules) ; le retard d'une frame ne gêne pas, le focus bouge lentement ;
   - la liste triée des meilleurs points d'intérêt, pour le budget d'émetteurs de [[Plan_AmbientLights]].

**Le cas de la caverne**

- Le soleil n'a pas de position : son point d'intérêt utile est **l'endroit où sa lumière entre**, la tache de soleil à la sortie.
- **Premier rebond** : un émetteur d'ambiance sur cette tache éclaire déjà l'intérieur de la caverne vers nous. C'est un vrai rebond, sans algorithme de chemin.
- **Rebonds suivants** (la lumière qui tourne un coin) : une chaîne d'émetteurs le long du chemin entre la sortie et le joueur, chacun plus faible que le précédent. C'est là que l'algorithme Bug servirait à trouver le chemin. Ce n'est plus physique, c'est un fil d'Ariane lumineux : à juger à l'œil.

## La palette du niveau comme indication (auteur, 2026-10-11)

**L'auteur** (qui confirme aussi la lecture ci-dessus « dans le principe ») : en PCG, si les niveaux sont générés avec une **ambiance / palette de couleurs**, cette palette **indique quoi faire ressortir**, ne serait-ce que comme couleur.

**Lecture de Claude**

- **La palette donne la couleur du rebond sans la mesurer.** C'était le point dur de [[Plan_AmbientLights]] (une trace ne donne pas la couleur de la surface touchée) : la couleur émise vient de la palette du lieu, pas d'une lecture de la scène.
- **La palette donne aussi un ingrédient du score** : ce qui s'écarte de la dominante ressort. En OKLCH, c'est un écart de teinte et de chroma, calculable ([[Guide_OKLab]]). Le citron est l'accent de la palette.
- **Tout vient de la même source** : le générateur qui choisit la palette choisit du même coup les accents, donc les points d'intérêt et leur couleur.

## Questions ouvertes

- **La palette** : quelle forme (dominante, secondaires, accent en OKLCH ?), et qui la porte (le lieu, le niveau, le graphe de [[Plan_DreamPlaces]]) ?
- **Un seul focus ou plusieurs** (un principal, des secondaires) ?
- **Quels effets** le focus pilote-t-il en premier : saturation, contraste, densité de particules, éclairage ?
- **Le score** : quels ingrédients, et qui les règle ?
- **Rapport avec la caméra et le combat** ([[Plan_3C]]) : le focus suit-il aussi une cible verrouillée ?
- **« Bug algo en GPU »** : sens confirmé par l'auteur dans le principe (2026-10-11). Reste à voir si le chemin doit vraiment être cherché sur GPU (un chemin par frame se calcule très bien sur CPU).
