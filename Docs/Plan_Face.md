# Plan — Visages des PNJ

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_Eyes]], [[Plan_NPC]], [[Plan_NormalTransfer]], [[Plan_KlattVoice]], [[Catalogue_Features]]

> Statut : 💭 **idée**, « une vague idée, il faut la mettre au point » (auteur, 2026-10-17). Rien n'est décidé, rien n'est codé.

## Intention (auteur, 2026-10-17)

- **Un visage expressif sur un mesh presque rigide.** Piste partagée avec Claude : l'expression vient en grande partie du shader. Les paupières de [[Plan_Eyes]] (plugin `SDFEyes`) en sont la première pièce.
- **Le mesh n'a pas à être totalement rigide** : à l'ancienne, on peut **mettre à l'échelle une partie du visage**, par exemple pour faire bouger la mâchoire.
- **Référence : l'animation en stop motion** (la manière dont certains films procèdent), avec un atout : l'outil maison de transfert de normales ([[Plan_NormalTransfer]]), « inclus dans les outils, facilement accessible avec le path en custom maintenant ».
- **Sourcils et bouche aussi**, la bouche pilotée par la synthèse vocale ([[Plan_KlattVoice]]).
- **Sourcils** : l'auteur doute qu'ils aient besoin de SDF, mais **sourcils et yeux devront fonctionner main dans la main**.
- **Textures** : de vraies textures, tout au plus générées de façon procédurale.
- **Vues visées** : « pense Skyrim, pense Deus Ex, Mass Effect aussi ». Donc des **dialogues de près**, avec des gros plans sur les visages des PNJ.
- **Antécédent** : de vieux tests de l'auteur dans Blender (perdus) : un mesh réaliste d'un seul bloc, avec de légers creux pour les orbites, et une parallaxe « étrange, naïve », sans SDF.

## Précisions de l'auteur (2026-10-17, suite)

- **Penser l'ensemble** : `SDFEyes` ([[Plan_Eyes]]) ne doit pas rester une île, il doit fonctionner avec le visage. Ce plan est l'endroit où l'on pense le tout.
- **Des yeux n'importe où, pas forcément par paire**, éventuellement un troisième œil (détail dans [[Plan_Eyes]]). Le visage ne suppose donc pas « deux yeux symétriques ».
- **Un éditeur de personnage complet** sera nécessaire.
- **Une tête modulaire** : mailler une tête entière est complexe, d'où l'idée de la construire en modules. Ça rejoint les pièces rigides de [[Plan_NPC]], la référence stop motion et le transfert de normales pour cacher les coutures, et peut-être l'outil de composition par formes ([[Plan_KitbashTool]]).

## Lecture de Claude (à valider)

- **Le stop motion par remplacement** (de mémoire, à vérifier) : des studios comme Laika impriment des milliers de visages et les remplacent image par image ; le visage est souvent coupé en deux, haut et bas, au niveau des yeux et du nez. L'équivalent ici : **des pièces de visage interchangeables** (bas du visage, mâchoire, bouche) qui se remplacent ou se mettent à l'échelle. La **couture** entre les pièces est cachée par le transfert de normales. C'est exactement le problème que cet outil résout déjà entre les pièces rigides du corps.
- **Ce que le shader prend en charge** : les paupières et le regard (`SDFEyes`), les sourcils (dessinés sur la surface, sans SDF a priori), les rides qui apparaissent avec l'expression, peut-être les détails de la bouche (lèvres, intérieur).
- **Bouche et synthèse vocale** : un synthétiseur de Klatt manipule des grandeurs proches de l'articulation (l'ouverture de la mâchoire suit à peu près le premier formant, l'arrondi des lèvres abaisse les formants suivants). Piste : **piloter la bouche directement par les paramètres du synthétiseur**, sans analyser le son. À vérifier dans [[Plan_KlattVoice]].
- **Gros plans de dialogue** (Skyrim, Deus Ex, Mass Effect) : c'est le cas le plus cher pour `SDFEyes`. La version analytique (étape U3 de [[Plan_Eyes]]) devient nécessaire, et la qualité doit tenir de près.
- **Vue subjective** : le joueur ne voit presque jamais son propre visage ; l'effort porte sur les PNJ.

## Expressions en couches (auteur, 2026-10-18)

**Critique du FACS apportée par l'auteur** : c'est un système **anatomique, musculaire et atomique** (AU1 = haussement de l'intérieur du sourcil, AU12 = coins des lèvres tirés…). Pour un sourire triste, il faut doser six curseurs à la fois, et les combinaisons créent souvent des interpénétrations laides, qui demandent des « corrective blendshapes ».

**Principe retenu par l'auteur** : découper le visage non pas en muscles, mais en
1. **Visèmes** (phonèmes visuels) : environ 10 à 15 formes pures pour la parole (`A`, `O`, `E`, `MBP`, `FV`, `LNTD`…) ;
2. **Macro-expressions, émotions primaires** : des poses complètes ou d'un demi-visage (joie, colère, peur, dégoût, surprise, tristesse).

**Lecture de Claude** :
- **La critique est juste pour l'animation à la main.** Les correctives viennent de ce que des blendshapes s'additionnent en ligne droite. Ici, les paupières ne peuvent pas se croiser (leur ordre est garanti par construction), mais les champs de déformation du mesh (les lèvres surtout) le peuvent encore.
- **Les deux niveaux se complètent, ils ne s'excluent pas.** Les pros animent avec des poses (visèmes, émotions), construites au-dessus d'un petit jeu de pilotes de bas niveau. Proposition : **ce qu'on anime** = visèmes + émotions ; **ce qui déforme** = un petit nombre de pilotes propres à nos modules (bien moins que les ~44 unités du FACS). Chaque pose est un préréglage de ces pilotes ; les correctives éventuelles se règlent une fois, dans la pose.
- **Les poses de demi-visage tombent juste avec le découpage** : masque haut (yeux, sourcils) et masque bas (bouche, joues). Un **sourire triste** = le haut de « tristesse » + le bas de « joie », sans doser six curseurs.
- Les six émotions primaires viennent elles aussi d'Ekman, et les visèmes ont leurs tables classiques (la parole des dessins animés, les 15 visèmes d'Oculus). Ce sont aussi des standards, simplement d'un niveau plus utile pour animer.
- La parole et l'émotion se superposent : les **visèmes pilotent la bouche** (depuis [[Plan_KlattVoice]]), l'**émotion pilote surtout le haut** et teinte le bas.

## Bouche, dents, langue (proposition de Claude, 2026-10-18, à discuter)

### Le même principe que les yeux, avec une différence

- **Les lèvres sortent du visage** et se voient de profil, comme les cils. Or un monde virtuel dessiné par le shader ne peut pas sortir de la surface du mesh. Les lèvres restent donc **du mesh** (masque bas et mâchoire), déformées dans le vertex shader : arrondi (O, U), étirement (E, I), fermeture (M, B, P), lèvre inférieure sous les dents du haut (F, V). La lèvre inférieure suit la **charnière de la mâchoire**.
- **L'intérieur de la bouche est dessiné par le shader**, comme le globe derrière les paupières. Une surface simple, cachée derrière les lèvres (le « fond de bouche »), porte un matériau qui lance des rayons dans un petit monde analytique : **dents, gencives, langue, cavité sombre**.
- **Garantie par construction** : tout ce monde est derrière cette surface, donc **les dents ne peuvent jamais traverser les lèvres**, un défaut classique des meshes de dents.

### Les dents

- Deux **arcades** (une courbe en fer à cheval chacune), avec des dents en formes simples arrondies placées le long de la courbe. L'arcade du haut est fixée au crâne ; celle du bas suit la charnière de la mâchoire.
- **Paramètres par personnage** : taille, écartement, alignement, dents manquantes, usure, couleur, **crocs** (créatures, animaux de [[Plan_NPC]]).
- **L'ombre des lèvres sur les dents et l'obscurité de la bouche** sont les indices de réalisme clés, comme l'ombre des paupières pour les yeux. Les pros ajoutent pour ça une coque d'occlusion ; ici, c'est un calcul dans le même petit monde.

### La langue

- Une forme analytique souple (une chaîne de quelques sphères fondues), avec peu de pilotes : **pointe levée** (L, T, D, N, derrière les dents du haut), **dos levé** (K, G), **avancée**, **abaissée** (voyelles ouvertes).
- Limite : la langue entre les dents (TH anglais) sort du monde virtuel ; cas rare, on l'accepte ou on avance un peu la surface du fond de bouche.

### La parole

- Le moteur d'articulation de [[Plan_KlattVoice]] (`KlattArticulator`) **connaît déjà chaque phonème et sa durée**. Les visèmes peuvent donc venir directement de lui, sans analyser le son, avec de l'**anticipation** : la bouche prend la forme d'un son un peu avant qu'on l'entende, et se fond entre voisins (coarticulation).
- Chaque visème est un préréglage de trois groupes de pilotes : **mâchoire** (ouverture), **lèvres** (forme), **langue** (position).

### Réponses de l'auteur (2026-10-18)

- **Dents** : le plan convient (« très bien ») ; **des formes simples suffisent**, pas besoin du niveau MetaHuman (« comme activer le RTX sur Control : on ne voit pas la différence »).
- **Animaux** : pas besoin d'être très rigoureux, **mais ça ne doit pas faire tache**.
- **Règle générale** : les humains lisent très bien les visages humains ; tant qu'on **évite la vallée dérangeante**, c'est parfait. Le jeu vidéo fait ça correctement depuis longtemps.
- Partage lèvres en mesh / intérieur en shader : l'auteur ne sait pas encore ; il demande **comment gérer des lèvres en mesh déformé**. Réponse ci-dessous.

### Les lèvres en mesh déformé : comment (proposition de Claude, 2026-10-18)

1. **Le maillage** : des **anneaux concentriques autour de la bouche** (la topologie classique des visages, qui suit le muscle orbiculaire), avec une **fente** entre les deux lèvres (deux bords séparés, qui coïncident bouche fermée). La lèvre supérieure appartient au masque bas, la lèvre inférieure à la mâchoire. Quelques anneaux (4 à 6) et 24 à 32 segments suffisent ; le coût du vertex shader est négligeable.
2. **Des coordonnées de bouche dans chaque sommet**, calculées une fois par un outil et rangées dans un canal d'UV ou les couleurs de sommet : position le long de la bouche (−1 coin gauche, +1 coin droit), distance à la fente, lèvre du haut ou du bas.
3. **Chaque pilote est un champ de déplacement** écrit dans ces coordonnées, et rapporté aux repères (coins de la bouche, charnière de la mâchoire), donc proportionnel à chaque visage :
   - **arrondi** (O, U) : les coins rentrent, les lèvres avancent ;
   - **étirement** (E, I) : les coins s'écartent et reculent ;
   - **fermeture et pression** (M, B, P) : les deux bords rejoignent la ligne médiane ;
   - **F, V** : le bord de la lèvre inférieure monte et recule sous l'arcade du haut, que l'on connaît (même repère que les dents) ;
   - **sourire** (bas du visage de l'émotion) : coins vers le haut et l'arrière, joues soulevées ;
   - **mâchoire** : la pièce tourne autour de sa charnière (transformation de la pièce entière, sans coût) ; la zone de transition sur le masque bas suit par le même champ, pour que la couture ne s'ouvre pas.
4. **Pas de lèvres qui se traversent, par construction** : l'ouverture de la fente est calculée d'abord (ouverture de la mâchoire moins la pression, jamais négative), puis les deux bords sont placés de part et d'autre. Le bord du haut reste toujours au-dessus de celui du bas, comme l'ordre garanti des paupières.
5. **Les normales** : un World Position Offset déplace les sommets mais pas leurs normales ; l'éclairage serait faux. Il faut **recalculer la normale dans le vertex shader** (dérivée du champ, ou deux évaluations voisines), comme le fait déjà l'outil de transfert de normales pour d'autres raisons.
6. **Les formes de l'éditeur de personnage** (épaisseur et largeur des lèvres…) utilisent la même machinerie : la forme et l'expression s'additionnent.
7. **Piège à vérifier** : les ombres en ray-march du projet utilisent les **champs de distance des meshes, calculés sur le mesh non déformé**. A priori, un World Position Offset n'y apparaît pas (`À VÉRIFIER` dans le source 5.8). Pour la bouche (petits mouvements), c'est sans gravité ; pour les **grosses déformations de l'éditeur de personnage**, l'ombre portée pourrait ne pas correspondre à la forme. À tester tôt.

### Idée de l'auteur : la bouche par la même méthode que les yeux (2026-10-18)

- Sur le piège des ombres : « la bouche ne se verra jamais en ray-march, détail trop petit, ce n'est pas grave ». (Le cas des grosses déformations de l'éditeur reste à tester.)
- **Idée de l'auteur** : faire la bouche essentiellement **par la même méthode que les yeux**, en déformant le mesh en dessous. Cette partie du visage est arrondie : le mesh pourrait avoir **une bosse** autour de la bouche, sous laquelle vit la bouche virtuelle.

**Lecture de Claude : oui, c'est l'extension logique**, et c'est mieux que des lèvres en mesh avec une fente :
- **Le mesh devient une enveloppe grossière qui bouge** : une bosse fermée, sans fente, déformée dans le vertex shader pour les grands mouvements (la mâchoire qui s'ouvre, l'avancée des lèvres pour O). C'est elle qui donne la silhouette.
- **Le shader dessine le détail à l'intérieur** : le bord des lèvres, la fente, les coins, le contact, puis les dents, la langue et la cavité, **dans un seul petit monde**. Il n'y a plus de fente à mailler, plus de « fond de bouche », et pas de normales à recalculer pour le détail des lèvres.
- **Les garanties de `SDFEyes` se retrouvent** : les lèvres ne se traversent pas (bord du haut toujours au-dessus du bas) et ne traversent pas les dents ; l'ombre des lèvres sur les dents et l'obscurité de la bouche sont calculées, avec un antialiasing analytique.
- **Code partagé avec les yeux** : un même monde virtuel du visage, en espace objet, avec des yeux et une bouche. Le nom du plugin pourra s'élargir le moment venu.
- **Coût** : la zone de la bouche fait à peu près la taille des deux yeux ; même ordre de grandeur.
- **Limite** : rien ne sort de l'enveloppe. Une moue très prononcée, une langue tirée, des défenses dépassent ; soit l'enveloppe se déforme pour les contenir, soit ce sont des pièces de mesh.

### Aperçu « Bouche en enveloppe » (2026-10-18)

Démo interactive (WebGL, hors projet) : https://claude.ai/artifact/HzxHAsM8qgFAZMQDRgcob1. Enveloppe arrondie ; dessous, en SDF : lèvres (bourrelets le long de la fente), dents en deux arcades (celle du bas suit la mâchoire), langue de trois sphères, cavité sombre. Visèmes (A, É, I, O, OU, M-B-P, F-V, L-T-D-N, sourire), « bonjour » enchaîné, visages (humain, lèvres pleines, bouche fine, orc), vue des parties, mesure de vitesse. Testée en local par Claude avant publication.
- **Garantie ajoutée en testant** : dents et gencives sont **bornées par l'enveloppe** (intersection), donc elles ne peuvent pas sortir des lèvres.
- **Pièges rencontrés** : sans limite au-delà des coins, la fente creusait une rainure sur tout le visage ; la déformation de la mâchoire doit rester locale (autour de la bouche) et douce aux coins ; elle déforme les distances, d'où des pas de marche plus prudents.
- Limites : visage plan autour, visèmes réglés à l'œil, défenses d'orc absentes (pièce de mesh à part).

**Verdict de l'auteur (2026-10-18) : « c'est horrible. »** Juste : l'aperçu était naïf, construit sans aucune base (des bourrelets le long d'une fente, sans arc de Cupidon, sans philtrum, une bouche en boîte). L'auteur donne les pistes de départ ci-dessous. L'aperçu reste en ligne comme trace, il ne fait pas foi.

### Bouche, v2 : une base sérieuse (pistes de l'auteur, lecture de Claude, 2026-10-18)

Pistes données par l'auteur : les **templates déformables de bouche en paraboles** ; la **segmentation des lèvres par modèle paramétrique en courbes cubiques** (Caplier) ; les **SDF en lobes** (Inigo Quilez).

- **Templates déformables** (Yuille, Hallinan, Cohen, CVPR 1989 puis IJCV 1992) : un trait du visage est décrit par un gabarit paramétré, ajusté à l'image en minimisant une énergie. L'œil = deux paraboles pour les paupières et un cercle pour l'iris ; la bouche = des paraboles pour les contours des lèvres (détail exact du gabarit de bouche `À VÉRIFIER` dans l'article). Intérêt ici : **peu de paramètres lisibles**, et c'est déjà le principe de nos profils de paupières.
- **Modèle de lèvres de Caplier** (Eveno, Caplier, Coulon 2004 ; Stillittano et Caplier 2008) : contour **extérieur** défini par des **points clés**, et relié par des **cubiques** et une **ligne brisée** pour l'arc de Cupidon. De mémoire, à vérifier dans l'article de 2004 : les deux coins ; les deux sommets de l'arc de Cupidon et le creux du philtrum entre eux (la ligne brisée) ; le point le plus bas de la lèvre inférieure ; des tangentes horizontales aux sommets et au point bas. Contour **intérieur** : deux cubiques et une ligne brisée bouche fermée, **quatre cubiques bouche ouverte** (confirmé par le résumé de Stillittano et Caplier 2008).
- **Lobes** (smooth min d'Inigo Quilez) : les lèvres ne sont pas des boudins uniformes. La lèvre supérieure a un **tubercule central** et deux masses latérales, l'inférieure **deux masses latérales**. On les modélise en lobes (ellipsoïdes) fondus par un smooth min, posés le long des contours.

**Construction proposée** :
1. **Contours 2D** (vue de face, dans le repère de la bouche) : extérieur façon Caplier (points clés, cubiques, ligne brisée de l'arc de Cupidon) ; intérieur (l'ouverture) en paraboles ou cubiques, **toujours contenu dans l'extérieur** (ordre garanti, comme pour les paupières).
2. **Volume 3D** : les lobes donnent l'épaisseur et l'avancée ; la surface rouge s'enroule vers le contour intérieur ; la limite rouge / peau suit le contour extérieur (léger bourrelet clair).
3. **Le relief autour** : les deux crêtes du philtrum qui montent depuis les sommets de l'arc, le sillon sous la lèvre inférieure, l'amorce du menton et des sillons naso-labiaux.
4. **Les visèmes déplacent les points clés** (coins, sommets de l'arc, ouverture intérieure) au lieu de gonfler des bourrelets.

**Méthode** (leçon de l'aperçu raté) : avancer par étapes vérifiables. D'abord les **contours 2D seuls** (points clés modifiables, formes de bouches variées, visèmes), validés à l'œil par l'auteur, **puis** le volume.

**Architecture de la bouche (auteur, 2026-10-18)** : comme l'œil est fait du globe et des paupières, la bouche est faite de **quatre couches indépendantes** : **cavité buccale + langue + dents et gencives + lèvres**. Les dents ne sont **pas accrochées aux lèvres** : celles du haut tiennent au crâne, celles du bas à la mâchoire ; les lèvres ne font que les découvrir.

**Retour de l'auteur sur l'étape 1** : « pas mal pour un début ». Les dents dessinées suivaient les lèvres : **non**. Les **commissures** (coins) ne lui plaisent pas : trop pointues.

**Étape 1, contours 2D (2026-10-18)** : démo « Contours des lèvres » (https://claude.ai/artifact/TtWK9UMwYNdMkv4McaNSxB), testée en local. Points clés P1 à P6 déplaçables, cubiques avec tangentes horizontales, ligne brisée de l'arc de Cupidon, contour intérieur fermé (avec le tubercule) ou ouvert (quatre cubiques), ordre garanti par échantillonnage, visèmes, sourire et tristesse, six bouches dont un orc, dents indicatives, copie des réglages. **En attente de la validation de l'auteur à l'œil**, avant le volume.
- **Mise à jour (2026-10-18)** : l'aperçu dessine maintenant les **quatre couches** dans l'ordre (cavité, langue, dents et gencives, lèvres). Les dents du haut sont fixes (crâne) ; celles du bas et la langue suivent **la mâchoire, pas les lèvres** ; une pointe de langue réglable (L, T, D, N). Commissures retravaillées : le rouge s'estompe dans la peau près des coins, petit creux d'ombre et pli qui prolonge la fente ; coins intérieurs presque confondus avec les coins extérieurs bouche fermée.

**Inquiétude de l'auteur (2026-10-18)** : « c'est un peu mieux », mais il a vraiment peur que ça ne puisse pas fonctionner en 3D, et demande quelle méthode a été utilisée et pourquoi.

**Réponse de Claude** :
- **Méthode utilisée** : le modèle de Caplier, un modèle de **vision par ordinateur pour segmenter des lèvres dans une image vue de face**. Choisi parce que l'auteur l'avait cité, et qu'il décrit bien la silhouette de face avec peu de paramètres.
- **La crainte est fondée** : c'est un modèle 2D. Il ne dit rien de la profondeur : l'avancée des lèvres, l'enroulement du rouge, les coins qui reculent le long de l'arcade dentaire, le contact entre les lèvres. Plaqué sur du 3D, il donnerait l'effet autocollant : **le même piège que les paupières en 2D** de l'auteur.
- **Le prolongement 3D existe** (laboratoire ICP de Grenoble) :
  - **Guiard-Marigny, Adjoudani et Benoît** (1994-1996) : les contours intérieur et extérieur du rouge sont décrits en 3D par des équations algébriques, ajustées sur 22 formes de référence et prédites par **environ cinq paramètres** ; le **contact des lèvres** est géré par un modèle volumique en **surfaces implicites**, la famille des SDF. Leur modèle 2D précédent : trois paramètres (largeur et hauteur de l'ouverture intérieure, avancée au contact).
  - **Revéret et Benoît** (1998) : un modèle 3D de 30 points de contrôle, piloté par **trois paramètres articulatoires** tirés de dix formes clés.
  - **Revéret et Essa** : quatre paramètres, ouverture de la mâchoire, arrondi, fermeture, élévation des lèvres.
- **Transposition à notre méthode**, comme pour les paupières : les contours ne sont pas dessinés sur un plan, mais comme des **profils sur une surface de support courbe** qui suit l'arcade dentaire. Coordonnées : angle autour de l'arcade, hauteur, **avancée**. Les points clés de Caplier deviennent des points clés dans ces coordonnées, avec une avancée par contour (le 3D de l'ICP). Le volume est une surface implicite entre contour extérieur et contour intérieur, et le contact se règle par fusion implicite, comme à l'ICP.
- **Proposition** : lever le risque tout de suite avec un **test 3D minimal des lèvres seules** (sans dents ni langue), vu de face, de trois quarts et de profil, bouche fermée, ouverte et arrondie. Si ça tient, on continue ; la démo 2D reste l'éditeur de formes de face.

### Alternatives pour la bouche en 3D (demandé par l'auteur, 2026-10-18)

L'auteur préfère comparer les approches avant un test, et trouve que **la méthode de Quilez semble bien**.

| Approche | Principe | Pour | Contre |
|---|---|---|---|
| **Contours paramétriques 3D** (ICP Grenoble, Caplier + profondeur) | Contours du rouge décrits par des courbes en 3D, pilotés par quelques paramètres articulatoires | Fait pour la parole, peu de paramètres, validé par la recherche | Il faut encore construire un volume autour ; risque d'effet autocollant si les contours ne sont pas vraiment 3D |
| **Modelage direct en SDF, à la Quilez** | On sculpte avec des primitives (ellipsoïdes, capsules), fusionnées par smooth min, creusées par smooth max, déformées (courbure, torsion) | Vraiment 3D dès le départ, organique, dans la même famille que `SDFEyes` | Beaucoup de primitives à régler à l'œil ; il faut une couche de pilotage pour les animer ; les déformations abîment les distances |
| **Mesh classique** (topologie en anneaux, blendshapes ou WPO) | Ce que fait tout le monde | Outils connus, silhouette exacte | Abandonné pour ce projet (coutures, pas de skinning, contact des lèvres à gérer) |
| **Modèle appris** (Revéret : 30 points + analyse statistique de formes clés) | Les formes viennent de mesures sur un vrai locuteur | Réaliste pour la parole | Il faut des données (captures) |

**Recommandation de Claude : un hybride, le modelage de Quilez piloté par les paramètres de l'ICP.**
- **Le volume à la Quilez** : chaque lèvre est faite de **lobes** (lèvre du haut : le tubercule central et deux masses latérales ; lèvre du bas : deux masses), fusionnés par smooth min ; la fente est **creusée** par un smooth max ; les sillons et le philtrum sont de petites soustractions douces.
- **L'astuce qui fait tenir le 3D** (comme pour les paupières sur le globe) : on modèle dans l'**espace de l'arcade**. On « enroule » l'espace autour d'un cylindre qui suit l'arcade dentaire : des primitives droites dans cet espace épousent la courbure du visage, et les coins reculent naturellement vers l'arrière.
- **Le pilotage à l'ICP** : quelques paramètres articulatoires (ouverture de la mâchoire, largeur, arrondi, fermeture, élévation, avancée) déplacent et déforment les lobes. Les visèmes et les émotions sont des préréglages de ces paramètres.
- **Les contours de Caplier** ne sont plus dessinés : ils **émergent** de l'intersection des lobes et de la fente. La démo 2D reste une référence de formes vues de face.
- Le contact entre les lèvres et les garanties (dents bornées par l'enveloppe, bord du haut au-dessus du bas) restent valables.

### Texte apporté par l'auteur sur l'hybride (2026-10-18) et confrontation

L'auteur a collé un texte (d'origine non précisée) qui développe l'hybride. Résumé :
- **Repère de l'arcade** : θ = atan2(x, z − z_centre), abscisse le long des dents u = R·θ, profondeur r = √(x² + (z − z_centre)²) − R (distance à la face avant des dents). Les **cinq lobes** (trois en haut, deux en bas) vivent dans un espace presque plat (u, y, r) : le recul des commissures est gratuit, l'épaisseur des lèvres est constante le long de l'arcade, le philtrum (au-dessus du tubercule) et le sillon sous la lèvre inférieure s'alignent sans distorsion.
- **Pilotage par cinq paramètres articulatoires**, présentés comme ceux de l'ICP / GIPSA-lab (Benoît, Guiard-Marigny, Badin, Le Goff) : **JW** (rotation de la mâchoire : les lobes du bas descendent et reculent un peu), **LP** (avancée : les cinq lobes sortent le long de r et la largeur active diminue, pour le [u] et le baiser), **LL** (élévation et abaissement : les lobes du haut montent, ceux du bas descendent, découvrant les incisives), **W** (largeur : les lobes latéraux s'écartent en θ, et l'épaisseur diminue par conservation du volume, pour le sourire et le [i]), **LC** (contact et compression : seuil de fusion ou de coupe médiane pour [p], [b], [m]).
- **Identité figée, animation légère** : rayons, positions de repos et courbures des lobes définissent le personnage une fois pour toutes ; le shader ne reçoit que `float5(Jaw, Protrusion, Width, Elevation, Compression)`.

**Confrontation (Claude)** :
- **D'accord sur l'essentiel** : c'est exactement la recommandation ci-dessus, avec les formules. Le budget tient largement dans les Custom Primitive Data.
- **À vérifier** : les noms et la liste des cinq paramètres attribués à l'ICP ne sont pas confirmés par les sources lues (Guiard-Marigny parle de largeur, hauteur et avancée ; Revéret et Essa de mâchoire, arrondi, fermeture, élévation). La phrase « 40 constantes magiques » sur Quilez est une image, pas un fait sourcé.
- **Cinq paramètres ne suffisent pas pour tout** :
  - ils couvrent la **parole**, pas l'**expression** : il manque l'élévation et l'abaissement des coins (sourire, tristesse), l'asymétrie, et la lèvre du haut relevée seule (rictus) ;
  - **LL** lie le haut et le bas, alors que **F, V** demandent la lèvre du bas seule, remontée et reculée sous les dents du haut, et que M, B, P **roulent** les lèvres vers l'intérieur. Il faut donc des réglages séparés pour le haut et le bas, et un **enroulement**.
  - Proposition : les cinq paramètres de parole, plus un petit jeu d'expression (coins haut / bas par côté, lèvre du haut relevée, enroulement), en cohérence avec les poses par demi-visage.
- **Le repère de l'arcade n'est pas une isométrie** : loin du cylindre, une unité de u ne vaut pas une unité de distance (facteur (R + r)/R). Les distances SDF doivent être corrigées (ou les pas de marche prudents), comme on l'a appris avec les paupières.
- **L'arc de Cupidon ne « se crée » pas tout seul** avec trois lobes : la crête claire de la limite rouge / peau et les colonnes du philtrum demandent une mise en forme explicite (petites additions et soustractions douces).
- **Les garanties restent à écrire explicitement** : la fente creusée entre les lèvres (bord du haut toujours au-dessus du bas), et les dents bornées par l'enveloppe. Les dents et les gencives se placent elles aussi dans le repère de l'arcade, ce qui est naturel.

### Preview 3D « Lèvres sur l'arcade » (2026-10-18, l'auteur absent)

Demandée par l'auteur avant de partir (« je te laisse me faire une preview »). Démo : https://claude.ai/artifact/XGWdTwTSBaD9ssYsFbqt2H. Testée en local par Claude (face, trois quarts, profil ; repos, A, sourire, OU, triste, M B P, F V, O de profil, orc).
- **Construction** : deux arcades cylindriques (u le long des dents, y, r avancée), celle du bas portée par la mandibule autour d'une charnière, rotation effacée vers les coins ; lèvres en **coupes elliptiques** le long de u (arc de Cupidon et tubercule en haut, deux coussinets en bas, effilement vers les coins) ; **conservation du volume** (largeur et sourire affinent, contact gonfle vers l'avant) ; **modiolus** (bosse aux coins, pli de la commissure plus marqué au sourire) ; fente creusée entre les deux repères (bord du haut toujours au-dessus, fin sillon même fermée) ; peau : philtrum, sillon sous la lèvre inférieure, menton ; dents du haut sur le crâne, dents du bas et langue sur la mandibule ; micro-plis en perturbation de normales.
- **Paramètres** : parole (JW, LP, W, lèvre du haut, lèvre du bas, LC, enroulement, F-V), expression (coins triste / sourire, asymétrie), identité (rouge haut et bas, arc, tubercule, largeur, avancée au repos, menton, micro-plis). Visèmes et expressions préréglés, « bonjour », cinq visages, vue des parties avec la grille de l'arcade et le modiolus, mesure de vitesse.
- **Retour de l'auteur (2026-10-18)** : « pas mal ». Les **micro-plis procéduraux du rouge sont atroces** (« un curseur vallée dérangeante, ça aurait été pareil ») : coupés par défaut dans la démo ; ils viendront de **textures** (wrinkle maps), comme décidé. Les **plis au bord des lèvres** lui plaisent ; c'est voulu : le pli de la commissure du modiolus (et sa bosse), qui part du coin, descend et s'écarte, plus marqué au sourire.
- **Constat de Claude** : en 3D, les formes se lisent bien comme des lèvres sous tous les angles testés, contrairement au premier aperçu. Restent : visage autour trop simple (un cylindre), couleur du rouge un peu pâle sous la lumière forte, O de profil un peu trop avancé, visèmes non comparés à des photos.

### Cible de qualité (auteur, 2026-10-18)

L'auteur a montré une série d'images : cinématiques et démos techniques des années 2000, rendus de personnages, plus *Les Indestructibles* et *Baldur's Gate 3*. Ce sont **une cible de qualité, pas des références de validation** : selon lui, ces images ne sont pas assez bonnes pour mesurer quoi que ce soit, et récupérer de vrais turnarounds de visages serait **moralement douteux**. *Les Indestructibles* et *Baldur's Gate 3* sont **à part**, aux deux extrêmes (très stylisé et très réaliste).

Ce que ces images ont en commun (lecture de Claude) : l'ombre fait la forme (sous la lèvre inférieure, sillon du menton, creux des commissures, philtrum) ; le sillon naso-labial porte beaucoup d'expression ; paupières épaisses qui ombrent l'œil, bord humide, cils sombres ; peau qui diffuse la lumière. Des volumes justes passent même très stylisés ; la vallée dérangeante vient de détails réalistes sur des volumes faux.

Références de validation, piste de Claude : **des photos que l'auteur prend lui-même** (son visage ou celui de proches consentants), face, trois quarts et profil, au repos et pour chaque visème et expression. Pas de problème moral, et c'est cohérent avec l'idée d'une capture au téléphone plus tard.

### Ressources libres et documentation NVIDIA (recherche du 2026-10-18, suggestion de l'auteur)

**Documentation NVIDIA, gratuite en ligne** :
- **« Dawn »** (démo de la GeForce FX, SIGGRAPH 2003), *GPU Gems* 1 : [chapitre 3, la peau](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-3-skin-dawn-demo) et [chapitre 4, l'animation](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-4-animation-dawn-demo), par Curtis Beeson et Kevin Bjorke. Points utiles ici : **au plus cinq blend shapes actifs à la fois**, en mélangeant **trois visages émotionnels environ, plus de petits modificateurs** (c'est notre principe des poses et des pilotes) ; la tête séparée du corps ; un **terme d'occlusion dynamique qui change selon que les yeux et la bouche sont ouverts ou fermés** (ce que nos SDF calculent directement).
- **« Human Head »** (Doug Jones, GeForce 8), *GPU Gems 3*, [chapitre 14, rendu réaliste de la peau en temps réel](https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch14.html), d'Eugene d'Eon et David Luebke : BRDF physiques, translucidité, diffusion sous la surface. Référence pour l'éclairage maison de la peau ([[Plan_Lights]]).
- [Les blend shapes sur GPU (Tristan Lorach, 2007)](https://developer.download.nvidia.com/SDK/10.5/direct3d/Source/GPUBlendShapes/doc/GPUBlendShapes.pdf), avec Dawn comme cas d'étude.

**Scans de têtes gratuits** (aucune ressource libre de droits trouvée pour les expressions) :
- [Eisko](https://www.cgchannel.com/2015/02/download-eiskos-free-hi-res-3d-scan-of-a-female-head/) : Creative Commons **non commercial** ;
- [3D Scan Store, base mesh multi-expressions gratuite](https://www.3dscanstore.com/blog/multi-expression-basemesh) : usage **privé ou non commercial** ;
- [Nexdata (Hugging Face)](https://huggingface.co/datasets/Nexdata/50_People_3D_Scanning_Face_Data/blob/main/README.md) : CC BY-NC-ND (non commercial, pas de dérivés) ;
- [FaceVerse](https://physics.paperswithcode.com/dataset/faceverse) : 21 expressions, licence à vérifier ;
- payants avec licence commerciale : Triplegangers, RenderHub.

**Décision de l'auteur (2026-10-18)** : il ne pourra pas se baser sur de vrais visages (pas de photos de visages réels comme base). Les **ressources libres serviront aux tests**, comme base neutre, et **ne seront pas gardées à la fin**. Précaution de Claude : leurs licences étant non commerciales, les garder **hors du dépôt** (dossier local non commité), uniquement pour regarder et comparer, et les supprimer avant toute diffusion du projet.

Nouvelles images de qualité montrées par l'auteur : la démo « Human Head » de *GPU Gems 3* (2007, presque 20 ans) et des rendus et sculptures récents (pores, grain du rouge des lèvres, poches sous les yeux, pattes d'oie, rides du front). Lecture de Claude : tout ce grain fin relève des **textures** (normales, wrinkle maps), comme décidé ; la SDF doit porter les **volumes** (sillons, plis, poches), pas le grain.

**Usage proposé** : ces scans peuvent servir de **références visuelles pour comparer** (les regarder à côté de nos rendus), **jamais intégrés au jeu ni utilisés pour en dériver de la géométrie**, puisque leurs licences excluent l'usage commercial. Les photos de l'auteur restent la référence la plus sûre.

### Approfondissement : les trois murs et les trois lois (apport de l'auteur, 2026-10-18)

**Trois murs** si l'on pose bêtement cinq lobes rigides sur un cylindre :
1. **L'effet « saucisses »** : la chair n'est pas une addition de billes. Quand la bouche s'étire (sourire, grand A), les lèvres **perdent de l'épaisseur**, s'aplatissent contre les dents et s'effilent vers les coins. Des lobes à rayon fixe font du plastique tubulaire.
2. **Le cylindre rigide** : l'arcade est plutôt une parabole (ou une caténaire tronquée), et **la mandibule bouge par rapport au crâne**. Avec un seul repère, la lèvre du bas se découple des dents du bas. **Il faut deux arcades.**
3. **Le modiolus** : le nœud fibreux des coins, où convergent environ huit muscles (zygomatiques, buccinateur, orbiculaire…). Il s'ancre, rentre dans la joue, tire et plisse les tissus autour. C'est là que la plupart des modèles procéduraux échouent.

**Trois lois** pour les dépasser :
1. **Conservation du volume** : quand la largeur (W) augmente, le rayon transversal des lobes diminue (R = R0 / (1 + k·ΔW)) ; quand les lèvres se compriment (LC, pour P, B, M), elles **gonflent un peu vers l'avant** pour compenser l'aplatissement vertical.
2. **Deux demi-repères articulés** : un demi-baril supérieur lié au maxillaire (fixe par rapport au crâne), un demi-baril inférieur lié à la mandibule, qui pivote autour de l'**articulation temporo-mandibulaire** ; raccord aux coins par interpolation angulaire. La lèvre du bas suit alors le recul des dents du bas.
3. **Les micro-plis du rouge** (*sulci labiorum*) : dans le repère de l'arcade, ce sont des fonctions périodiques le long de u ; ils se resserrent et se creusent à la compression, et s'effacent presque quand la lèvre s'étire.

**Réponses de l'auteur aux questions ouvertes** :
- **Aspect des lèvres (6)** : **en textures**, avec des normales pilotées par l'animation, comme les chevaux de *Red Dead Redemption 2*. Une partie reste dans le shader (rugosité, humidité…), paramètres de toute façon présents.
- **Lien avec `SDFEyes` (7)** : l'auteur n'a pas tout le pipeline en tête. Proposition de Claude ci-dessous.
- **Validation (8)** : d'accord sur les trois vues fixes et les visèmes de référence, **plus des approximations faites sur des références** (photos).

**Lecture de Claude** :
- Les trois lois se rangent bien dans le pipeline : volume et deux arcades dans la fonction SDF ; micro-plis en **wrinkle maps** (textures de normales mélangées selon les paramètres, la technique des chevaux de RDR2 et de beaucoup de visages de jeux), pilotées par W et LC, comme le propose la loi 3.
- **Le modiolus** se modélise comme un **point d'ancrage par coin** dans le repère de l'arcade : les lobes s'y effilent ; il recule et monte avec le sourire (zygomatique), descend avec la tristesse ; il gonfle un peu la joue autour (déformation de l'enveloppe) et creuse le pli de la commissure.
- Le recours aux textures pour les rides (point 6) rejoint « de vraies textures, tout au plus procédurales » plus haut.

**Organisation proposée (point 7)** :
- **Un plugin, deux traits du visage** : les yeux et la bouche sont deux fonctions distinctes, qui partagent une **bibliothèque commune** (départ du rayon, éclairage maison, antialiasing, ombres, vue des parties).
- **Le découpage de la tête règle déjà la question** : le masque haut porte les yeux, le masque bas et la mâchoire portent la bouche. Chaque pièce a son matériau, qui n'évalue que le trait qu'elle porte : pas de coût en double.
- **La bouche est à cheval sur deux pièces** (lèvre du haut sur le masque bas, lèvre du bas sur la mâchoire) : les deux matériaux évaluent **le même monde de la bouche**, avec les mêmes paramètres. Chacun dessine ses pixels, et la couture ne s'ouvre pas : le principe du champ commun à la tête.
- Les **yeux n'importe où** restent possibles : n'importe quel mesh peut porter le trait « œil ».
- ~~Le nom `SDFEyes` pourra s'élargir quand la bouche y entrera.~~ L'auteur n'est pas d'accord (2026-10-18) : « ce n'est pas la même chose ». La bibliothèque commune lui paraît juste (« pas faux »). Le rangement (un plugin par trait, ou un plugin commun avec la bibliothèque) reste `À DÉCIDER`.

Sources : [Guiard-Marigny et al., 3D Models of the Lips (HAL)](https://hal-univ-tlse3.archives-ouvertes.fr/IMAG/inria-00537531) · [ICPhS 1995](https://www.coli.uni-saarland.de/Phonetics/icphs/ICPhS1995/13_ICPhS_1995_Vol_3/p13.3_222.pdf) · [Revéret et Benoît 1998](https://www.cs.bu.edu/fac/betke/cs591/papers/reveret-benoit.pdf) · [Animated Speakers (Georgia Tech)](https://cpl.cc.gatech.edu/projects/animated-speakers/index.html) · [cours UMD sur les modèles de lèvres](https://www.cs.umd.edu/class/fall2019/cmsc828X/LEC/lips.pdf)

Sources : [Yuille et al., CVPR 1989](https://mlanthology.org/cvpr/1989/yuille1989cvpr-feature) · [IJCV 1992](https://doi.org/10.1007/bf00127169) · [gabarit d'œil en deux paraboles (BMVC 1991)](https://bmva-archive.org.uk/bmvc/1991/bmvc-91-030.pdf) · [Stillittano et Caplier, VISAPP 2008](https://www.scitepress.org/Papers/2008/10774) · [Quilez, smooth minimum](https://iquilezles.org/articles/smin)

### Ce qui est faisable comme bouches (Claude, 2026-10-18)

| Bouche | Faisable ? | Comment |
|---|---|---|
| Humaines variées (lèvres fines ou épaisses, large, étroite) | Oui | Paramètres de forme, comme les profils des paupières |
| Prognathisme, mâchoire avancée ou reculée | Oui | Position de la pièce mâchoire et de l'arcade du bas |
| Grosses dents, dents écartées ou manquantes, crocs qui restent dans la bouche | Oui | Arcades paramétrées |
| **Orc** : bouche large, lèvres épaisses, mâchoire inférieure massive en avant | Oui | Enveloppe plus large, arcade du bas avancée |
| **Défenses d'orc** (qui sortent et reposent sur la lèvre du haut) | Avec une pièce | Un mesh accroché à la mâchoire, posé devant les lèvres, comme les « head parts » de Skyrim |
| Crocs de vampire qui dépassent sur la lèvre | À la limite | Dans l'enveloppe s'ils dépassent peu ; sinon une petite pièce |
| Gueules animales (museau, longue mâchoire) | Oui, sans rigueur | Arcade allongée, enveloppe en museau |
| Langue tirée, moue extrême | Avec limite | L'enveloppe se déforme, ou on accepte la coupure |
| Bec, mandibules d'insecte | Non par cette méthode | Pièces rigides animées |

### Questions

- ~~Lèvres en mesh déformé, intérieur en shader ?~~ → l'auteur propose plutôt **la bouche entière en SDF sous une enveloppe déformée**. Approche à valider. `À DÉCIDER`
- ~~Les créatures et les animaux : parler ou s'ouvrir ?~~ → pas besoin d'être rigoureux, mais ça ne doit pas faire tache.
- ~~Niveau de détail des dents~~ → formes simples.

## Questions ouvertes

- Les pièces du visage : quel découpage (haut / bas, mâchoire, bouche) ? Remplacement de pièces, mise à l'échelle, ou les deux ? `À COMPLÉTER`
- Les sourcils : forme dessinée, brins, ou texture déformée ? Avec quels pilotes communs avec les paupières (plissé, surprise, colère) ? `À COMPLÉTER`
- La bouche : quelle part en géométrie (mâchoire), quelle part en shader (lèvres) ? `À COMPLÉTER`
- La variété des visages : faits à la main, générés, ou générés puis retouchés ? `À COMPLÉTER`
- ~~L'éditeur de personnage : pour qui ?~~ → **pour l'auteur ET pour le joueur** (auteur, 2026-10-17). Conséquence : il doit tourner **en jeu**, donc sans outils réservés à l'éditeur Unreal.
- ~~Les modules de la tête~~ → proposition de l'auteur (2026-10-17) : **haut de la tête, masque, mâchoire, oreilles et nez**, avec peut-être quelques détails en plus. Avis de Claude et méthode des pros ci-dessous.

## Découpage de la tête : avis de Claude (2026-10-17, à valider)

**Ce que font les pros** (de mémoire, à vérifier avant de s'en servir) :
- **Skyrim, Mass Effect, Deus Ex, MetaHuman** : **un seul mesh de visage**, avec la **même topologie pour tout le monde**. L'éditeur de personnage déforme ce mesh par des curseurs (morph targets, parfois des os du visage mis à l'échelle). Des pièces s'ajoutent par-dessus : yeux, dents, langue, cheveux, barbe, sourcils, cicatrices (les « head parts » de Skyrim). MetaHuman Creator mélange des visages scannés **par régions** (front, yeux, nez, bouche, mâchoire), mais toujours sur une seule topologie.
- **Pourquoi** : une seule topologie permet à **toutes les animations faciales de marcher sur tous les visages** (un sourire bouge la bouche, les joues et les paupières inférieures en même temps, à travers les régions) ; pas de couture qui s'ouvre quand le visage se déforme ; une texture continue.
- **Le découpage en pièces** existe surtout là où le visage ne se déforme pas : le stop motion (haut et bas du visage séparés au niveau des yeux, pour remplacer la bouche sans toucher aux yeux) et les jeux qui échangent des accessoires.

**Pour DWAM, ton découpage est cohérent**, parce que les raisons des pros ne s'appliquent pas : pas de skinned mesh, expression surtout portée par le shader (paupières, sourcils), coutures cachées par le transfert de normales. Ajustements proposés :
1. **Placer les coutures là où elles se cachent** : la lisière des cheveux (haut / masque), sous la mâchoire, la jonction des oreilles. Éviter les joues et le front, où l'œil lit les reflets.
2. **La mâchoire tourne autour d'une charnière** (sous l'oreille), en plus de l'échelle : c'est le mouvement réel de l'ouverture.
3. **Le masque en deux, haut et bas** (comme le stop motion), si la bouche et les joues doivent changer de forme sans toucher aux yeux : un masque haut (front, arcades, orbites, pommettes, arête du nez) qui porte les yeux et les sourcils ; un masque bas (lèvre supérieure, sillons, bas des joues) qui accompagne la bouche.
4. **Nez et oreilles comme pièces posées et interchangeables**, à la manière des « head parts » : c'est là que la variété se voit le plus.
5. **Les formes dans l'éditeur** : sans skinned mesh, Unreal n'a pas de morph targets sur les static meshes. Deux pistes : déformer dans le **vertex shader** (World Position Offset piloté par les curseurs), ou **générer le mesh une fois** à la création du personnage (Dynamic Mesh à l'exécution). ~~À trancher~~ → **vertex shader** (décision de l'auteur, 2026-10-18).

**Décision de l'auteur (2026-10-18)** : les cinq ajustements sont **acceptés**, et les formes se déforment **dans le vertex shader**.

## Les expressions marchent sur tous les visages, proportionnellement (2026-10-18)

Question de l'auteur : « c'est pareil pour nous, non ? Juste proportionnellement ? » Réponse de Claude : **oui, et c'est même un point fort, à une condition.**

- **Une expression est un jeu de paramètres, pas un déplacement de sommets.** « Sourire à 70 % » donne les mêmes nombres pour tout le monde, appliqués à la forme propre de chaque visage. Elle s'adapte donc proportionnellement, sans retargeting. Les pros, eux, doivent garder une topologie commune pour obtenir ça.
- **La condition : un sourire traverse les modules.** Il remonte les coins de la bouche (masque bas), soulève les joues (masque haut, pommettes) et remonte les paupières inférieures (`SDFEyes`). Il faut donc :
  1. **définir chaque expression comme un ensemble coordonné** de paramètres sur tous les modules et sur `SDFEyes` ;
  2. **calculer les déformations dans un champ commun à la tête**, évalué de la même façon par chaque pièce, dans les coordonnées de la tête. Deux pièces voisines bougent alors exactement pareil à leur couture, qui ne s'ouvre pas ;
  3. **exprimer ces champs par rapport à des repères du visage** (ancres des yeux, coins de la bouche, charnière de la mâchoire), pour qu'ils suivent les proportions de chaque visage.
- **Vocabulaire proposé : le FACS** (*Facial Action Coding System*, Ekman et Friesen), la référence pour décrire les expressions en « unités d'action ». Exemple de mémoire : sourire franc = AU6 (soulève les joues) + AU12 (tire les coins des lèvres). Chaque unité d'action devient un pilote qui touche plusieurs modules à la fois.
- ~~**L'auteur valide le FACS**~~ → nuancé par l'auteur juste après, voir « Expressions en couches » ci-dessous.
- Note d'origine : l'auteur valide le FACS (2026-10-18, « je pensais aller là-dessus »). Contexte (Claude, de mémoire) : publié en 1978, révisé en 2002 ; vieux, mais toujours la référence. Les 52 blendshapes d'ARKit, qu'utilise Live Link Face sur iPhone, en sont inspirés : piste pour **piloter un jour les visages par capture avec un téléphone**. Limites : le FACS décrit des mouvements visibles, pas des émotions, et ne couvre ni la parole (les visèmes viennent à part, depuis [[Plan_KlattVoice]]) ni la langue.
