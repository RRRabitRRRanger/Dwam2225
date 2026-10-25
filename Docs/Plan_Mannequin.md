# Plan — Le mannequin : assembler un personnage

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_NPC]], [[Plan_Face]], [[Plan_Eyes]], [[Catalogue_Features]], [[Plan_NormalTransfer]]

> Statut : ✅ **plan validé par l'auteur** (2026-10-21, « très bon plan »), avec l'ajout des outils de forme (M7). **M1 codé et compilé** (2026-10-21), pas encore testé dans l'éditeur : [[Guide_CharacterKit]].

## Intention (auteur, 2026-10-21)

- Avant les fiches de PNJ, ou en parallèle : **pouvoir faire un mannequin**, comme dans n'importe quel éditeur de personnage.
- C'est **sur lui qu'on mettra les yeux, la bouche**, puis les vêtements.
- Les yeux : « un personnage avec des yeux, pas fait de manière stupide », et **pouvoir en placer où je veux**.

## Références de l'auteur (2026-10-21)

Deux familles d'images : des **figurines articulées** (S.H.Figuarts « body-kun », Romankey × Cowl, poupée à coques de type Synthetic Human / poupée articulée) et des **planches de construction** (yeux, lèvres, nez, dents, mâchoire, épaule ; dessin coréen « PROPIC × TACO », Julia Acherontias, MadeFun3D, Tatami).

Ce que Claude en retient pour le mannequin :

- **La figurine articulée est exactement notre modèle** : un squelette, des pièces rigides sculptées, échangeables. Le corps est découpé en volumes simples (cage thoracique, bassin, membres), comme sur la planche de Tatami.
- **Les pièces se chevauchent au lieu de se toucher** : coques qui glissent l'une sous l'autre (dos, hanches, épaules), rotules cachées sous un bord. C'est la manière la plus simple de cacher une jointure sans déformer : la pièce voisine passe *sous* le bord. À combiner avec le transfert de normales.
- **Le tronc en plaques empilées** : une plaque par os de la colonne (`spine_01` à `spine_05` du mannequin), qui se recouvrent comme une carapace. Répond à la question « une pièce sur plusieurs os » : non, plusieurs pièces qui se chevauchent.
- **Les doubles articulations** (coude, genou à deux pivots) : une petite pièce intermédiaire entre deux os, qui suit à moitié chacun. Le mannequin d'Unreal a des os de torsion (`upperarm_twist`, `lowerarm_twist`, `thigh_twist`…) : ils peuvent porter ces pièces de liaison.
- **L'épaule à part** (planche TACO : épaule, poitrine, bras, dans cet ordre) : le deltoïde est une pièce propre, entre clavicule et bras, pas le haut du bras.
- **Les mains à phalanges** : une pièce par phalange (le mannequin a les os des doigts) ; c'est beaucoup de pièces, d'où l'intérêt des instances plus tard.

Ce qu'elles confirment pour le visage ([[Plan_Face]], [[Plan_Eyes]]) :

- Les lèvres se construisent **en plans, sur l'arcade dentaire**, la lèvre du haut recouvre celle du bas ; virage aux canines → notre approche « lèvres sur l'arcade ».
- La mâchoire s'ouvre **en arc** autour de sa charnière : de face, le menton raccourcit → la pièce mâchoire en charnière.
- L'œil : iris ≈ un tiers de la largeur, la hauteur du coin externe par rapport au coin interne donne l'humeur (= `Tilt`), le seul flux de la paupière supérieure varie beaucoup les regards, les cils suivent la normale du globe → c'est ce que fait SDFEyes.
- Le nez en primitives planes → une pièce de nez échangeable.

### Deuxième série (mains, avant-bras, jambes) et décision

- **Décision de l'auteur (2026-10-21) : jointures invisibles.** En 3D, les pièces **peuvent s'interpénétrer** : pas besoin des astuces mécaniques des figurines (coques, rotules). Le rendu naturel vient de deux pistes : un **vrai data transfer** (normales transférées sur le mesh, comme dans Blender) et le **transfert de normales en shader via les Mesh Distance Fields** (celui de l'auteur). Les chevauchements de la section précédente restent utiles pour la forme, pas pour cacher.
- Les MDF suivent leur pièce rigide sans retard (le retard documenté concernait le GDF) : bonne nouvelle pour cette piste.
- **La supination** : les figurines détaillées ont deux tiges dans l'avant-bras (radius, ulna). Chez nous : les os de torsion de l'avant-bras portent une pièce qui tourne à moitié, le muscle « suit le pouce » (planche Arms p.146).
- **La main** : paume en plaques de métacarpes qui s'écartent en éventail (Kotobukiya / Kagami), coussin du pouce à part, poignet en rotule ; une pièce par phalange. Proportions des planches (cercles aux articulations, paume ≈ carré, pouce en diagonale) : base pour générer des mains de tailles différentes. Os de métacarpes du mannequin 5.8 à vérifier.
- **Le muscle qui change avec l'angle** (biceps tendu / plié) : une pièce rigide peut quand même **changer d'échelle selon l'angle** de l'articulation voisine (correctif bon marché).
- **Le genou** : rotule en pièce séparée entre cuisse et tibia (pièce de liaison).

## Ce qui est déjà décidé (voir [[Plan_NPC]])

- **Un seul squelette** : le mannequin d'Unreal (`Content/Characters/Mannequins/Meshes/SK_Mannequin`), déformé pour les autres bipèdes et les quadrupèdes. Os ajoutés (queue, oreilles) si besoin.
- **Des pièces rigides attachées aux os** : le squelette est une armature animée invisible, chaque partie du corps est un mesh statique attaché à un os.
- **Un profil de déformation par groupes** (jambes, bras, buste, cou, tête, queue) : longueur et épaisseur, reliés aux os réels par une petite table.
- **Le transfert de normales cache les jointures** (comme le snake), avec les voisines passées à chaque pièce plutôt que le GDF (qui retarde sur ce qui bouge).
- Les vêtements : swap de pièces à la Skyrim ; les retirer = « déballer » ([[Catalogue_Features]]).

## But final

Un **asset de personnage** qui décrit tout le corps (pièces, profil, ancres), assemblé à l'exécution sur le mannequin animé, et **deux interfaces sur le même cœur** : un outil dans l'éditeur Unreal pour l'auteur, un écran de création en jeu pour le joueur.

## Les briques

| Brique | Rôle |
|---|---|
| `FCharacterDescription` (données) | **La description d'un personnage** : pièces (pour chaque **emplacement** : mesh, os, offset, matériau), plus tard profil, ancres, curseurs. Copiable, modifiable, sauvegardable : c'est ce que modifient l'outil d'éditeur, l'écran du joueur, les sauvegardes, les PNJ générés |
| `UCharacterPreset` (asset) | Un **préréglage** qui contient une description de départ. En jeu les assets sont en lecture seule : on modifie une copie, jamais l'asset (décision du 21/10) |
| `UBodyProfile` (asset ou struct) | Les facteurs longueur / épaisseur par groupe d'os |
| Ancres | Des points nommés posés sur une pièce (os + offset) : `Eye`, `Mouth`, plus tard attaches de vêtements. Un œil = une ancre ; autant d'ancres qu'on veut, où on veut |
| `UCharacterAssemblyComponent` | Lit l'asset, crée les composants de pièces attachés aux os (`AttachToComponent(..., BoneName)`), applique le profil, crée les composants d'yeux sur les ancres |
| Profil appliqué aux os | Dans l'AnimBP (nœud *Transform (Modify) Bone*, `FAnimNode_ModifyBone`) ou la Control Rig ; IK des pieds plus tard |

API vérifiées dans le source 5.8 : `USceneComponent::AttachToComponent` (attache à un nom d'os comme socket), `FAnimNode_ModifyBone`, `UPrimaryDataAsset`, plugins `ControlRig` et `IKRig` présents.

Nom de plugin provisoire : **`CharacterKit`** (neutre, sans « Dwam »). À confirmer.

## Étapes

Chaque étape se teste dans l'éditeur avant la suivante.

1. **M1 — Assembler depuis un asset, sans interface.** Plugin `CharacterKit`, `FCharacterDescription`, `UCharacterPreset`, `UCharacterAssemblyComponent`. Un Blueprint avec le mannequin (mesh caché ou visible au choix) + le component : les pièces (cubes, capsules du moteur pour commencer) suivent les os pendant l'animation idle et la marche. *Critère : on change une pièce dans l'asset, on la voit changer.*
2. **M2 — Le profil de déformation.** Groupes d'os, longueur et épaisseur, appliqués dans l'AnimBP. *Critère : un personnage grand et un trapu avec les mêmes animations ; pieds au sol à peu près (IK plus tard).*
3. **M3 — Les ancres et les yeux.** Ancres dans l'asset ; un component d'œil SDFEyes par ancre, avec son petit volume invisible et un asset de forme d'œil (`UEyeShape`) au lieu des 28 entrées câblées. *Critère : deux yeux sur la tête, puis un troisième sur l'épaule, sans toucher à la tête.* Remplace les étapes U2 (câblage manuel) et U5 de [[Plan_Eyes]].
4. **M4 — Les jointures.** Transfert de normales entre pièces voisines. *Critère : le coude et le cou ne montrent plus de couture franche.*
5. **M5 — L'outil d'éditeur.** Un panneau (Editor Utility Widget ou outil C++) : choisir une pièce par emplacement, régler le profil par curseurs, déplacer les ancres à la souris, aperçu animé. *Critère : faire un personnage sans ouvrir l'asset à la main.*
6. **M6 — L'écran du joueur.** Même cœur, interface en jeu, curseurs nommés (« paupière lourde », « long cou ») qui écrivent dans l'asset. Plus tard.

7. **M7 — Les outils de forme** (ajout de l'auteur, 21/10). Aider à **dessiner les pièces**, avec des guides **qui servent vraiment** (pas les « cadres de composition » décoratifs qu'on trouve en ligne) :
   - **Éditeur seulement** : générateurs de pièces fines — phalanges, métacarpes, muscles (un profil balayé le long de l'os, gonflement réglable), à affiner ensuite à la main.
   - **Aussi en jeu** : nez, forme de la tête, proportions du visage, par curseurs.
   - **Des règles mesurables, pas des lignes** : les proportions sont des **contraintes avec une valeur typique et une plage réelle**, tirées de l'anthropométrie (mesures de Farkas sur des populations), les canons de dessin servant de valeurs par défaut. Exemples tirés des références : tiers du visage (racine des cheveux – sourcils – base du nez – menton), yeux à mi-hauteur de la tête, largeur d'un œil ≈ écart entre les yeux ≈ largeur du nez, bouche ≈ écart des pupilles, oreille entre sourcils et base du nez. Le curseur peut sortir de la plage (orc, stylisé), mais le guide montre où est le « naturel ».
   - **Des pièces construites comme les dessinateurs les construisent** : nez = un pont en trapèze + une boule de pointe + deux ailes (planche « Схема построения носа ») ; lèvres = trois lobes en haut, deux en bas, la crête, le bord net, la partie plate qui s'enroule (Anatomy for Sculptors) — c'est déjà notre bouche en lobes sur l'arcade. Ces constructions sont des paramètres naturels pour des pièces SDF ou générées.
   *Critère : un nez et une tête réglés par curseurs, que l'auteur trouve justes ; une phalange générée qu'il garde.*

Ensuite : la bouche ([[Plan_Face]]), les vêtements, la fiche de PNJ qui référence l'asset de personnage.

## Questions ouvertes

- Les **emplacements** : liste fixe (tête, cou, buste, bassin, bras, avant-bras, mains, cuisses, jambes, pieds) ou libre (n'importe quel os) ? Proposition : libre, avec une liste par défaut.
- ~~Une pièce sur plusieurs os~~ → plaques qui se chevauchent, une par os (références figurines).
- ~~Jointures visibles ou cachées~~ → **invisibles** ; pièces qui s'interpénètrent, data transfer + transfert de normales MDF (auteur, 21/10).
- Le « vrai » data transfer : fait une fois dans l'éditeur (pose de repos) ou recalculé quand les voisines bougent ? À creuser à l'étape M4.
- Les pièces de test : formes du moteur, ou un premier kit fait par l'auteur ?
- Le nom du plugin.
- **Les recherches de l'auteur** sur sa façon de faire (à venir) : elles priment sur ce plan.

## Lot du 2026-10-25 : ratios, rotules, mélange des normales (✔ vérifié par Claude, captures)

- **M2 — Profil de corps** (`FCharacterBodyProfile` dans la description) : taille, longueur des jambes, des bras, du tronc, du cou ; taille de la tête ; épaules, cage, taille, bassin ; épaisseur des bras et des jambes. Les **longueurs** passent par le squelette (`UCharacterKitAnimInstance`, sans Blueprint : l'os parent s'allonge, le bassin monte ou descend pour garder les pieds au sol) ; les **épaisseurs** par les pièces. Six préréglages tirés de la planche de torses : Standard, Enfant, Mince, Lourd, Musclé, Grand. Test : pieds au sol à ±1,2 cm pour tous.
- **Rotules** aux épaules, coudes, poignets, hanches, genoux, chevilles (option `bIgnoreLength` : une pièce qui ne s'allonge pas avec l'os). 35 pièces, symétrie 0,01 cm.
- **Mélange des normales** : matériau de test `Content/_Claude/M_Claude_JointBlendDebug` (créé par script, `Tools/Claude/create_joint_blend_material.py`), quatre modes : hybride (deux sondes GDF), normale d'origine, masque en orange, méthode « surface » de l'auteur.
  - **Le champ global suit le personnage qui marche** : sur trois images consécutives, le masque reste collé aux pièces, pas de traînée visible à cette échelle. La variante du snake n'est donc **pas** nécessaire (décision de l'auteur : ne la tester qu'en cas d'échec).
  - **R2 = 40 cm est trop large pour un corps** : tout le personnage est dans le mélange. **R2 ≈ 12 cm** concentre le masque aux vraies jonctions (bras contre le buste, entre les cuisses, chevilles).
  - **Le mélange reste discret aux coudes** : la précision du GDF (de l'ordre de 10 à 20 cm près de la caméra) est du même ordre qu'un avant-bras. ~~Pistes : affiner le GDF, ou capsules par os.~~ **Ni l'un ni l'autre** (auteur, 2026-10-25). Cause probable vue ensuite : les formes du moteur sont mises à l'échelle de façon très inégale (cylindre 0,1 × 0,1 × 0,4), ce qui fausse leur champ de distance.
- Pièges de test notés : le shader ne compile qu'au premier rendu (capture de chauffe) ; les MDF se construisent en arrière-plan (`BlockUntilAllBuildsComplete`) ; une Scene Capture n'a de GDF que si `bAlwaysPersistRenderingState` est vrai.
- **Suite (2026-10-25), à la manière de l'auteur** : matériau unlit avec le soleil calculé dans le shader, albédo beige, capture **brute** (couleur de scène avant le post-process d'Unreal, encodée en sRGB par le test), bonhomme **× 2,5** (idée de l'auteur). Résultats : au genou, la coupure entre cuisse et tibia s'efface nettement en hybride ; mais le mélange **aplatit aussi les formes rondes** (le bras devient une planche, la rotule de l'épaule disparaît) et **fusionne les deux jambes** entre elles. La méthode « surface » reste très proche de l'original. Le XOR ne se lit pas encore (taille de cellule à revoir).
- **Test des deux sphères (demande de l'auteur, 2026-10-25), mesure de la distance lue par la sonde** : à 5 cm, la sonde lit à peu près la bonne distance (avec un quadrillage, la trace des cases du GDF) ; à 25 et 60 cm, elle lit **à peine 20 à 30 %** de la distance réelle. **Le champ global d'Unreal est tronqué** : il ne connaît les distances que dans une bande étroite autour des surfaces (une dizaine de centimètres ici) et renvoie une valeur plafonnée au-delà. La variante hybride (`DwamNormalTransferHybrid`) suppose une vraie distance à 40 cm : elle croit voir une jonction **partout**, d'où le mélange « qui porte loin ». L'auteur, qui mélangeait **près de la surface**, voyait le mélange surtout aux intersections : c'est cohérent. L'intersection elle-même est bien détectée (distance négative = dans l'autre sphère).
- **Correction (auteur, 2026-10-25) : ça mélange bien.** La planche « mesure » n'était pas une méthode de mélange, seulement la distance brute lue par la sonde. Comparaison des méthodes sur les deux sphères (côté loin, côté près, dessus) : la variante **champ** (une sonde, 15 cm) adoucit surtout le **creux de l'intersection** ; l'**hybride** adoucit plus largement (contours, haut de la grosse sphère) ; la méthode **surface** (alpha 0,5) reste discrète. Le « masque partout » de l'hybride vient de la troncature du GDF, mais loin de l'intersection la normale cible reste proche de celle du mesh : l'effet visible y est faible.
- **Méthode de l'auteur relue dans `MF_Light2` (2026-10-25, lecture seule par export T3D, `Tools/T3DTrace.ps1`)** : normale = `lerp(normalize(gradient GDF à la surface), normale du mesh, AO_Great)`, avec `AO_Great = smoothstep(Param_16, dfsgdfghd, d(D)/D × lerp(0.25, 1, d(D+1)/(D+1)) × lerp(0.25, 1, d(D+2)/(D+2)))`, trois sondes le long de la normale à `Distance2/22/222` = 10 / 11 / 12 cm. Le mélange est donc **piloté par la distance** (une pseudo-AO à trois sondes), pas par un alpha fixe : c'était l'erreur de la version précédente (`SurfaceAlpha` 0,5, retiré du test). Reproduite fidèlement dans `M_Claude_JointBlendDebug` (mode 3, paramètre `AuthorDist`).
- **À réglages égaux** (sonde 10 puis 25 cm, mêmes vues) : la méthode de l'auteur et la variante « champ » adoucissent toutes deux **le creux de l'intersection** ; l'hybride sans part de base aussi, un peu plus large. Les résultats à 10 et 25 cm sont **presque identiques** : au-delà d'une dizaine de centimètres, le GDF tronqué renvoie la même valeur, donc pousser la sonde ne change plus rien.
- **Pièces générées avec MDF (2026-10-25, idée de l'auteur)** : `Tools/Claude/create_proxy_parts.py` (Geometry Script) fabrique les 35 pièces à leur vraie taille, arrondies (capsules et ellipsoïdes, échelle cuite dans la géométrie), dans `Content/_Claude/Parts/` ; le test `CharacterKit.Export.ProxySpecs` lui donne les tailles. Les 35 ont leur MDF (vérifié par le test). (Correction, auteur : l'effet des formes étirées sur le champ n'est pas démontré ; le gain réel est d'avoir des pièces arrondies à leur taille.) Résultat à sonde égale (10 cm) : à taille normale, **ta méthode et le champ adoucissent les jonctions** (coudes, genoux, bassin) ; défauts vus : **pieds assombris** par ta méthode (sol tout proche : c'est le défaut du bas du corps que l'auteur connaissait, l'hybride le règle par le rejet du sol), **stries** sur l'intérieur des cuisses (champ, hybride), là où deux pièces se frôlent. À × 2,5 les trois méthodes sont propres et discrètes.
- **Sonde à l'échelle du corps** (×2,5, sonde 25 cm) : le genou et le haut des cuisses fondent maintenant comme le torse ; les épaules un peu plus. Confirme que la largeur du fondu est en cm : **la sonde doit suivre la taille du personnage** (le profil de corps pourra la régler, par exemple via Custom Primitive Data).
- **Idée de l'auteur (2026-10-25) : un outil de réglage de la sonde**, sur le modèle de son outil de travail pour les tailles de lightmap (réglage en partie automatique + prévisualisation de debug). Ici : régler la distance de sonde par objet ou par personnage (déduite de sa taille, retouchable), avec une vue de debug du mélange (le mode « masque » / « jonctions » du matériau de test) directement dans le viewport. À faire avec l'éclairage maison, quand le transfert de normales y sera branché.

## M3 — Ancres et yeux (2026-10-25, ✔ vérifié par Claude, captures)

- **Ancres** dans la description (`FCharacterAnchor` : nom, os, décalage, type `Point` ou `Eye`, forme d'œil, rayon du globe, miroir). Le component crée un component par ancre, attaché à son os ; `AddAnchor` en ajoute une à l'exécution ; `GetAnchorComponent` la retrouve. Le profil de corps s'applique (tête plus grosse → yeux déplacés et agrandis).
- **Yeux par défaut** posés par `MakeProxyDescription` sur l'ellipsoïde de la tête ; `UCharacterKitLibrary::MakeAnchor` place une ancre depuis le repère du personnage et déduit le côté du nez.
- **SDFEyes** : nouveau module `SDFEyesRuntime` — `USDFEyeShape` (asset de forme, les 17 réglages de la démo), `USDFEyeComponent` (sphère support à 1,2 × le globe, matériau dynamique réglé chaque image, **micro-vie** : clignements tous les 2 à 6 s avec rebond, micro-saccades de 0,5 à 1°). Plus aucun câblage à la main : le matériau `Content/_Claude/M_Claude_SDFEyes` est généré par `Tools/Claude/create_sdfeyes_material.py` (les 28 entrées du guide U2).
- **Test `CharacterKit.Visual.Eyes`** : deux yeux sur la tête, nez opposés ; ouvert, mi-clos, fermé, regard de côté, trois quarts, profil ; un clignement arrive tout seul en 8 s ; **un troisième œil sur le bras**, sans toucher à la tête.
- **À reprendre (vu sur les captures)** : la peau des paupières est rose (éclairage de démo) alors que le corps est beige ; la sphère support dépasse de la tête comme un œuf, faute de vraie orbite sculptée ; les cils sont à peine visibles à cette distance. Brancher l'éclairage maison et la couleur de peau de la pièce porteuse (étape U4/U5 de [[Plan_Eyes]]).
- **Corrections après la recette de l'auteur (2026-10-25)** : (1) l'œil semblait **coupé horizontalement** : au-delà des coins, les deux paupières se rejoignaient sur la ligne de fermeture et leurs bords arrondis creusaient un sillon tout autour du globe (invisible dans la démo, qui ne montrait pas au-delà des coins) ; corrigé dans `Eyelids.ush` (une seule peau juste après les coins). (2) **Regard de côté** : le miroir inversait aussi l'axe du regard, les yeux divergeaient ; `Gaze.X` suit maintenant le même côté pour tous les yeux. (3) Les yeux d'un même acteur partagent leur micro-vie (clignent ensemble, vérifié par le test). Reste : les cils, trop fins pour se voir à cette distance.

## Références de l'auteur pour des pièces sur mesure (2026-10-25)

Pour un futur générateur de pièces « plus guidé » que les capsules (M7), « pas vraiment le moment » selon l'auteur :
- **Plans du visage peints sur une vraie tête** (front, tempes, pommettes, nez, bouche, menton, mâchoire en aplats de couleur) : un découpage directement réutilisable pour une tête en pièces ou en plans.
- **Le coude de figurine qui se plie mal** (la fente du biceps) : exactement ce que le chevauchement et le transfert de normales doivent éviter.
- **Topologie de main par doigt** (couleurs par phalange, paume, pouce à part) : une pièce par phalange, comme prévu.
- **Muscles du bras, de la jambe, de l'épaule, de la cuisse** (avec repères numérotés) : les volumes à suivre pour des pièces de membres galbées au lieu de capsules droites.
- **Planches TACO** : le pouce construit en prisme triangulaire, l'intérieur de la cuisse plus saillant (coupe en goutte, pas en cercle), la ligne de hanche selon le genre, le pli de la taille entre côtes et bassin, la taille des ongles (moitié de la dernière phalange ; pouce le plus grand, auriculaire le plus petit), la forme du nez qui reprend celle du visage.

## M4 — Jointures branchées (2026-10-25, ✔ vérifié par Claude, captures)

- **Matériau de corps** `Content/_Claude/M_Claude_Body` (généré par `Tools/Claude/create_body_material.py`) : la méthode de l'auteur (`MF_Light2`, trois sondes → pseudo-AO → `lerp(gradient, normale, AO)`) en permanence, plus le **rejet du sol** de la variante hybride, et le garde-fou « ne jamais retourner la normale ». Éclairage provisoire dans le shader en attendant l'éclairage maison. Paramètre `DebugBlend` pour voir le fondu en orange.
- **La largeur du fondu suit la taille** : `FCharacterBodyProfile::JointBlendDistance` (10 cm pour un corps de taille 1) × `Height` × échelle de l'acteur, envoyée par le component à chaque pièce en **Custom Primitive Data 0**. Vérifié par le test `CharacterKit.Visual.BodyJoints` : Enfant 6,2 cm, Standard 10, Grand 12,5, Standard × 2,5 → 25.
- **Limite vue** : sur le géant (25 cm), presque tout le corps est dans le fondu, parce que le champ global d'Unreal est tronqué au-delà d'une dizaine de centimètres (mesuré sur les deux sphères) : la distance lue plafonne et la pseudo-AO croit qu'une pièce est proche partout. Le rendu reste propre, mais le fondu y est moins sélectif. Le bas des pieds garde un peu de fondu malgré le rejet du sol.
- Correction annexe : un œil sans matériau restait une sphère d'un mètre (il sortait avant de régler sa taille) ; il est maintenant caché et à la bonne taille.
