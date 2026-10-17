# Plan — Paupières dessinées sans mesh (et yeux procéduraux)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_NPC]], [[Plan_NormalTransfer]], [[Guide_OKLab]], [[Catalogue_Features]]

> Statut : 💭 **idée**. Source : le doc de l'auteur « Plan de jeu UE5 - Systèmes et Features » (Claude Docs, 2026-10-16), puis précisions de l'auteur (2026-10-17). Rien n'est décidé, rien n'est codé.

## Le vrai sujet : les paupières (auteur, 2026-10-17)

> « Quand je parle d'yeux, les LLM comprennent globe oculaire, alors que ça, c'est simple et documenté. Moi, je veux les paupières : les dessiner, faire la shadow, etc., **sans mesh**. »

- **Le globe oculaire n'est pas le sujet** : iris, cornée, sclère sont bien documentés (modèle « Eye » d'Unreal, etc.). Les publications sur le globe ne l'intéressent pas.
- **Le sujet** : des paupières **dessinées par le shader**, avec leur ombre sur l'œil, sans mesh de paupière.
- **Fourche tranchée** : ~~tout-shader ou hybride (paupières en mesh skinné)~~ → **sans mesh** (auteur, 2026-10-17). Cohérent avec « pas de skinned mesh » ([[Plan_NPC]]).

## Ce que le doc du 2026-10-16 disait déjà

- Paupières : masques en courbes + ménisque + petit AO ; **le même clignement** pilote la fermeture et le drain de couleur.
- Un component pilote : regard, dilatation, clignement, larme, couleur, drain. Effet fantaisie : la couleur quitte l'iris pour teinter la larme.

## Ce qu'il faut dessiner (lecture de Claude, à valider)

1. **La forme** : le bord de la paupière supérieure et celui de l'inférieure, chacun une courbe pilotée (ouverture, inclinaison, forme en amande, coin interne / externe). Le clignement anime ces courbes.
2. **L'épaisseur du bord** : la tranche de la paupière (la ligne d'eau), plus claire et humide, et le pli au-dessus.
3. **L'ombre sur le globe** : la paupière supérieure est une coque un peu plus grande que le globe et couvre sa partie haute. Son ombre est une bande douce sous le bord, qui dépend de la lumière (direction du soleil et des lumières maison).
4. **Le contact** : un AO fin le long du bord et le ménisque de larme (un liseré brillant).
5. **Les cils** (`À COMPLÉTER` : voulus ou non) : des traits le long du bord, avec leur ombre.

## La question qui décide tout : le relief

Une paupière dépasse du globe. Sans mesh, deux façons :

- **A. Peinte sur le globe** : on dessine la paupière sur la surface de la sphère de l'œil, comme un masque avec ombrage. C'est simple et bon marché. Mais la silhouette reste celle du globe : de profil, la paupière n'a pas d'épaisseur. C'est l'approche des styles « anime ».
- **B. SDF dans un volume** : la paupière est une forme analytique (une coque de sphère découpée par les courbes du bord), ray-marchée dans un petit volume autour de l'œil. Elle a un vrai relief, une silhouette et une ombre exacte. C'est plus cher, mais c'est seulement un œil par personnage, et la scène est proche de la technique des ombres GDF du projet.

Proposition de Claude : commencer par **A** pour régler la forme et le clignement, puis passer à **B** si la silhouette de profil compte. Les courbes du bord et le component de pilotage servent dans les deux cas.

**Décision de l'auteur (2026-10-17)** : commencer par **A**, peinte sur le globe ; le relief viendra après.

## Étape A : mise en œuvre (proposition de Claude, en attente de validation)

> ⚠️ **Rejetée par l'auteur (2026-10-17) : la base est fausse.** Ses corrections :
> - « Le mesh ne bouge pas, le shader oui » : le regard fait bien rouler l'œil dans le shader.
> - **Pas de sphère** : l'œil se dessine **sur n'importe quel mesh**. Son prototype reconstruit le globe à partir des UV, sans géométrie (`Content/AVOLABO/Shaders/OKLCH/Shader_Eyes` et `Shader_EyeLids`).
> - **A1 et A2 ont déjà été essayées par l'auteur**, qui a fini par tout recommencer. Ce plan les répète.
>
> Lecture du prototype par Claude (HLSL des nœuds Custom) : deux yeux symétriques par `abs(UV.x) - EyeSpacing` ; un globe reconstruit en 3D à partir des UV (z = √(1 − r²)) ; un regard qui fait rouler l'œil (yaw / pitch, iris et cornée tournés, réfraction ou parallax de l'iris) ; des paupières en courbes 2D dans les UV (vesica piscis, puis paraboles avec ouverture, skew, tilt, et `LidThickness`, `EyeShadow`, `OffsetBlink`). La suite ci-dessous reste comme trace, elle ne fait pas foi.

### Le principe qui simplifie tout : le globe ne tourne pas

Le mesh du globe (une simple sphère) reste **fixe dans la tête**. Le regard est un **paramètre du shader** : c'est l'iris qui se déplace sur la sphère, pas le mesh. Une sphère qui tourne ne se voit pas, sauf par son iris. Conséquence : les paupières, attachées à la tête, se calculent directement dans l'espace local du mesh, sans transformation.

### Les coordonnées

Pour chaque pixel du globe, on prend la direction depuis le centre de la sphère (position locale normalisée) et on la convertit en deux angles : **azimut** (gauche-droite) et **élévation** (haut-bas), mesurés depuis l'axe du regard au repos.

### La forme du bord (A1)

- **Bord supérieur** : une courbe élévation = f(azimut), nulle aux deux coins (interne et externe) et haute au milieu. Réglages : ouverture, position du sommet (plus près du nez ou de la tempe), inclinaison des coins (yeux en amande, tombants, relevés), arrondi.
- **Bord inférieur** : même courbe, plus plate.
- **Clignement** : un paramètre 0..1 qui fait descendre le bord supérieur jusqu'à une ligne de fermeture, pendant que l'inférieur remonte un peu.
- **Masque anti-aliasé** analytiquement (avec `fwidth`, comme `Patterns.ush`) : pas de crénelage sur le bord, sans TAA.

### L'ombrage (A2)

- **Peau de la paupière** : là où le masque couvre le globe, on dessine la peau. Sa normale est celle de la sphère, **roulée vers l'avant près du bord** : ça suggère l'épaisseur sans relief réel.
- **Ligne d'eau** : une bande fine au bord, plus claire et humide, avec un petit spéculaire.
- **Pli** au-dessus du bord supérieur : une ligne d'ombre douce, à une distance réglable.
- **Ombre de la paupière sur le globe**, qui suit la lumière : on traite la paupière supérieure comme une coque un peu plus grande que le globe. Pour un point du globe, on lance le rayon vers le soleil, on calcule où il sort de cette coque, et on regarde si ce point de sortie est « dans la paupière ». La distance au bord à cet endroit donne la pénombre. C'est analytique, donc exact et bon marché : une intersection rayon-sphère.
- **AO de contact** : une bande sombre le long des deux bords, indépendante de la lumière.
- **Ménisque de larme** : un liseré brillant juste au-dessus du bord inférieur.
- Tout passe par `DwamShadeSurface` ([[Guide_Lights]]) : soleil et lumières maison, style OKLab.
- **Limite** : l'ombre de la paupière ne s'applique qu'au soleil ; les lumières maison n'ont pas d'ombre par lumière aujourd'hui. L'AO de contact les assombrit quand même.

### Le regard et l'iris (A3)

- Le regard est une direction (paramètre). L'iris est un disque provisoire placé sur la sphère dans cette direction, juste pour voir les paupières le recouvrir. Le vrai iris (et le drain de couleur vers la larme) viendra ensuite.
- Les paupières **suivent un peu le regard** (quand on regarde en bas, la paupière supérieure descend) : un réglage de couplage, comme chez l'humain.

### Le pilotage (A4)

Un component qui anime le clignement (intervalles aléatoires), oriente le regard vers une cible et écrit les paramètres dans un Material Instance Dynamic. Les deux yeux partagent le clignement.

### Où va le code

- `Eyelids.ush` dans `Plugins/DwamLighting/Shaders/` : le shader dépend de l'éclairage maison (`Shading.ush`).
- Le component (A4) : `À DÉCIDER` avec l'auteur (module `DwamLightingRuntime`, ou un plugin à part au nom neutre).

### Jalons

| Étape | Ce qu'elle apporte | Jalon |
|---|---|---|
| A1 | La forme et le clignement | Compilation hors moteur (DXC, FXC), puis l'auteur règle la forme sur une sphère, dans un Material Instance |
| A2 | L'ombrage complet | Faire tourner le soleil : l'ombre de la paupière bouge de façon cohérente sur le globe |
| A3 | Le regard | Regarder en haut, en bas, sur les côtés : l'iris passe sous les paupières, elles suivent un peu |
| A4 | La vie | Un PNJ cligne et suit une cible du regard, sans intervention |

Guide de montage pas à pas pour le matériau (nœud Custom, paramètres exacts), comme pour les autres shaders.

## Méthode « paupières en pivot » (proof of concept, 2026-10-17)

Démo interactive (WebGL, hors Unreal) : https://claude.ai/artifact/JYrwNRk6dQuzxMzprcQpos. Réaction de l'auteur à la v1 : « bluffant » ; la vue des parties est « super utile pour les réglages ».

- **Principe** : les paupières vivent dans le **même espace 3D virtuel que le globe**. Chaque pixel du mesh lance un rayon dans un petit monde analytique (globe, deux coques de paupière, plan du visage), sur n'importe quel mesh.
- **Un seul repère : l'axe des deux coins.** Bord supérieur, bord inférieur et pli sont des angles autour de cet axe. Rangés dans l'ordre (bas ≤ fermeture ≤ haut ≤ pli), ils ne peuvent se croiser qu'aux coins, quels que soient les réglages.
- **v1** : un plan par bord (coins + sommet). Défaut trouvé : le réglage « tension » courbait le plan, qui pouvait recouper la coque et percer la paupière (bosse noire).
- **v2** (testée en local par Claude avant publication) : chaque bord est un **profil de hauteur sur le globe** (élévation en fonction de l'azimut, mesurées depuis le centre du globe). ~~Angles autour de l'axe des coins~~ abandonnés pendant les tests : l'axe passe à l'intérieur du globe, donc la tranche de la paupière était coupée en biais (une bande sombre vue de face). Avec le centre du globe, la tranche est radiale, comme une vraie paupière. Garanties conservées : bas ≤ fermeture ≤ haut ≤ pli par construction, coins fixes.
- **Pièges de ray-march rencontrés** (utiles pour Unreal) : une distance en arc (r·écart d'angle) surestime la vraie distance et fait traverser la paupière au rayon, il faut r·sin(écart) ; le rayon doit partir d'une surface englobante (dans Unreal : la surface du mesh), et un rayon qui épuise ses pas doit être accepté au dernier point plutôt que rejeté.
- **Détails demandés par l'auteur** : cils, sourcils, chair du coin interne (caroncule), veines de la sclère, pupille qui réagit à la lumière (déjà dans son prototype, avec l'iris qui suit).

### Retours de l'auteur sur la v2 (2026-10-17)

- **Réglages par défaut de l'auteur** (devenus ceux de la démo, préréglage « Par défaut ») : `{"uEU":0.22,"uTU":0.24,"uPU":0.93,"uEL":-0.22,"uTL":0.15,"uPL":1,"uWidth":0.62,"uTilt":0.1,"uClose":0.47,"uFoldGap":0.23,"uFold":0,"uHood":0.5,"uThick":0.11,"uLash":1.5,"uCurl":0.32,"uCar":0.93,"uVeins":1}`.
- **Cils** : « on dirait qu'ils sont sur une sorte de cylindre », comme un mesh visible d'un seul côté. C'est exact : ce sont des brins découpés dans **une seule nappe**. À refaire plus tard (brins en 3D, plusieurs rangées, touffes).
- **Un cil du haut dédoublé**, dans un cas aléatoire extrême : `{"uEU":0.5,"uTU":-0.21,"uPU":1.98,"uEL":-0.4,"uTL":0.08,"uPL":0.63,"uWidth":0.52,"uTilt":-0.21,"uClose":0.25,"uFoldGap":0.38,"uFold":0.02,"uHood":0.33,"uThick":0.1,"uLash":1.44,"uCurl":1.2,"uCar":0.66,"uVeins":1}`. À examiner (piste : la nappe se replie quand la courbure est forte et le bord pointu).
- **Pupille énorme** → réduite (~30 % de l'iris en lumière moyenne). **Pli trop fort** → adouci ; l'auteur peut « faire avec ».
- **« Au hasard »** : bonne idée ; même les cas aberrants restent cohérents, seuls les plus extrêmes ont de vraies erreurs.
- La sclère : « pas parfaite, mais bluffante ».

### Conditions pour un jeu en temps réel (validées par l'auteur, 2026-10-17)

1. Couper tôt : test rapide dans les UV avant toute marche.
2. LOD selon la taille à l'écran : sous ~20 pixels, version sans marche.
3. Ombre des paupières : le soleil seulement.
4. Boucles bornées et courtes, sortie anticipée ; code dans un `.ush` du plugin, pas dans le nœud Custom.
5. Paramètres par personnage via **Custom Primitive Data** (pas un matériau par personnage). Le rendu reste dans le pixel shader : un compute shader n'apporte rien à la marche, qui dépend du point de vue de chaque pixel.

Jalon de performance : une dizaine de PNJ à l'écran en 1080p, mesuré avec `stat GPU` / `ProfileGPU`, budget à fixer (ex. 0,1 ms pour tous les yeux). Estimations de Claude, non mesurées.

## Implémentation Unreal (proposition de Claude, 2026-10-17, en attente de validation)

L'auteur est convaincu par le proof of concept (2026-10-17). Rien n'est codé.

### Principe dans le moteur

- **L'œil virtuel vit dans l'espace objet du mesh** : centre, rayon et orientation du globe sont des paramètres. Le mesh (la pièce rigide « visage ») ne fait que fournir les pixels ; partout où il couvre la zone de l'œil, le pixel lance un rayon dans ce petit monde. Ça marche sur n'importe quel mesh, à condition que **le monde virtuel reste derrière la surface du mesh** (voir les pièges).
- **Matériau unlit + nœud Custom** qui appelle une fonction d'un `.ush`, comme le reste de l'éclairage maison. Le point touché est éclairé par `DwamShadeSurface` (soleil + lumières maison, [[Guide_Lights]]).
- **Le globe** : celui du prototype de l'auteur (`Shader_Eyes` : iris, cornée, réfraction, pupille), porté dans le `.ush`. Les paupières sont la partie nouvelle.
- **Les paramètres de chaque personnage** passent en Custom Primitive Data : **36 floats maximum** (`NumCustomPrimitiveDataFloat4s = 9`, vérifié dans `SceneTypes.h` du 5.8). Budget prévu : forme ~17, animation 4 (clignement, regard x/y, pupille), repère de l'œil 4, côté ; on reste sous 36. Les formes elles-mêmes sont rangées dans un **Data Asset** par personnage.
- **Les deux yeux** : un seul jeu de paramètres, mis en miroir par le côté (comme `sideSign` dans le prototype de l'auteur).

### Les cils : une approche différente de la démo

Dans la démo, les cils sont **le point le plus faible**. Ils sont mal placés (vues de côté et paupière fermée, captures de l'auteur du 2026-10-17) et ce ne sont que des brins découpés dans une nappe. Et surtout, **ils dépassent du visage** : dans Unreal, tout ce qui sort de la surface du mesh ne peut pas être dessiné par le rayon, qui part de cette surface.

Proposition : **une petite bande de géométrie procédurale pour les cils seulement** (des quads fins), dont la position est calculée **dans le vertex shader à partir des mêmes profils** (`.ush` partagé, World Position Offset). Ce n'est pas un retour aux paupières en mesh : la bande ne porte que les cils, sans forme propre.
- Racine exactement sur l'arête avant du bord, direction vers l'avant du visage puis courbure ; longueur, densité et touffes par paramètres.
- Les cils suivent le clignement et le regard sans décalage, puisqu'ils lisent les mêmes profils.
- Le MSAA antialiase leurs bords (ce sont de vraies arêtes), ce que la démo ne pouvait pas offrir.

### Étapes (chacune testable)

| Étape | Contenu | Jalon |
|---|---|---|
| U1 | `Eyelids.ush` : portage de la démo (profils, coque, bord arrondi, pli, caroncule, veines, vue des parties) | Compilation hors moteur (DXC, FXC), comme les autres `.ush` |
| U2 | Matériau de test sur une sphère ou un plan, paramètres en Material Instance | L'auteur colle un JSON de la démo et voit la même image dans Unreal |
| U3 | **Version analytique** : intersection directe des deux sphères, marche réservée au bord arrondi ; ombre et AO analytiques ; antialiasing analytique des bords | Même image ; temps mesuré (`ProfileGPU`) contre la version en marche |
| U4 | Globe du prototype de l'auteur branché, pupille qui réagit à la lumière | Comparaison avec `Shader_Eyes` |
| U5 | Component (clignement, regard vers une cible, pupille, suivi du regard) + Data Asset + écriture en Custom Primitive Data ; **import d'un JSON copié depuis la démo** | Un PNJ cligne et suit une cible du regard ; la démo sert d'éditeur de formes |
| U6 | Cils en bande procédurale (WPO depuis les profils) | Cils plantés sur le bord, vus de face, de côté et paupière fermée |
| U7 | Coupure précoce, LOD selon la taille à l'écran, mesures | 10 PNJ en 1080p dans le budget fixé |
| Plus tard | Peau autour (cernes, poches, rides), rig ou drivers de l'auteur | — |

### Pièges connus (démo, recherche et moteur)

1. **Distances** : r·sin(écart), pas l'écart en arc, qui surestime la distance et fait traverser la paupière au rayon. Corriger par la pente du profil.
2. **Repère** : angles mesurés depuis le **centre du globe**. Un axe qui passe dans le globe coupe la tranche en biais.
3. **Départ du rayon** : vérifier qu'il ne démarre pas à l'intérieur d'un solide (dans la démo, cela donnait un anneau noir). Un rayon qui épuise ses pas est accepté au dernier point plutôt que rejeté.
4. **Volume du mesh** : le monde virtuel doit rester derrière la surface. La zone de l'œil du mesh doit suivre à peu près la coque extérieure des paupières, ou passer devant. La silhouette en vue rasante reste celle du mesh.
5. **MSAA** : il n'antialiase que les arêtes de géométrie, pas les bords calculés dans le shader (bord de paupière, iris). Il faut un antialiasing analytique (`fwidth` sur la distance au bord), sans TAA.
6. **Profondeur** : le point touché n'écrit pas sa profondeur. Pixel Depth Offset seulement si quelque chose doit croiser l'œil (mèches, mains), car il coûte, probablement en désactivant le test de profondeur précoce sur ces pixels (`À VÉRIFIER` dans le source).
7. **Cornée** : la coque doit passer au-dessus de la bosse de la cornée (`CorneaBump` du prototype), sinon la cornée traverse la paupière.
8. **Boucles dans le nœud Custom** : code dans le `.ush`, boucles bornées avec `[loop]` (FXC déroule sinon, compilation lente), sortie anticipée.
9. **Ombres** : l'ombre portée par le reste de la scène (GDF) part de la surface du mesh, pas du point virtuel. Le décalage est faible, à vérifier à l'œil. L'œil virtuel n'existe pas dans les champs de distance.
10. **Cas extrêmes** : un cil dédoublé dans un cas aléatoire (JSON plus haut). Les cils sont refaits de toute façon (U6).
11. **Matériel visé** : les mesures sont faites sur une RTX 5070 ; la carte minimale visée est `À COMPLÉTER`.

### Décisions de l'auteur (2026-10-17)

- **Système à part** : le plugin **`SDFEyes`** (nom choisi par l'auteur). Il inclut les `.ush` de `DwamLighting` pour l'éclairage.
- ~~Cils en bande procédurale~~ → **non**. ~~L'auteur a déjà réalisé des cils sans cette exception~~ (contresens de Claude). Ce que l'auteur voulait dire : il **avait déjà compris** que les cils qui sortent du mesh ne seraient pas dessinés, et il l'accepte, car c'est rare et limité aux angles de vue extrêmes. **Les cils restent dans le shader**, à refaire mieux que dans la démo (bien plantés sur le bord, vrais brins). L'étape U6 devient : cils en shader, bien placés, avec la coupure assumée au bord du mesh.
- **Placement de l'œil par component** : pratique dans l'éditeur. Précision de Claude : un socket vient de l'asset du mesh (placé dans l'éditeur de mesh) ; un component se place dans la scène ou le Blueprint, avec une poignée visible. Proposition : un component « ancre d'œil », enfant du mesh, dont la transformation relative est écrite en Custom Primitive Data. Il pourra suivre un os plus tard.
- **Carte visée** : la génération actuelle, **milieu de gamme à prix raisonnable** (ordre de grandeur : RTX 5060), en comptant sur de meilleures cartes plus tard.
- **LOD** : une simple projection 2D au loin, **réglable dans les options** du jeu (validé).
- **Des yeux n'importe où, en nombre libre** (auteur, 2026-10-17). Dans ses tests en shader, l'auteur dessine les deux yeux d'un coup : certaines choses sont mises en miroir, d'autres non, et il pensait l'activer par un switch. Mais il veut pouvoir **placer des yeux sur à peu près n'importe quoi, pas forcément par paire**, et peut-être un troisième œil. Conséquences (lecture de Claude) :
  - **un œil = une ancre** (le component prévu), en nombre libre ; une paire n'est qu'un cas particulier (deux ancres, la seconde en miroir de la première) ;
  - **un miroir par réglage**, pas global : ce qui se reflète (forme, coins, inclinaison) et ce qui ne se reflète pas (le regard vers une cible, la lumière) ;
  - **la limite de 36 floats de Custom Primitive Data** ne suffit pas pour plusieurs yeux sur le même mesh. Piste : ranger les yeux dans une **texture de données**, comme les lumières maison ([[Plan_Lights]] : quelques pixels par élément, écrite par un subsystem). Le matériau ne lit que les yeux de son mesh.
  - L'étape U5 sera réécrite en conséquence.
- **Ne pas faire de SDFEyes une île** (auteur, 2026-10-17) : il doit fonctionner avec le visage. On réfléchit à l'ensemble dans [[Plan_Face]].

### La micro-vie : ce qui évite le regard de poupée de cire (apport de l'auteur, 2026-10-18)

Le piège des personnages automatisés est le **regard statique de poupée de cire**. Pour l'éviter sans animation à la main :
- **Clignements** : un minuteur aléatoire en C++, toutes les 2 à 6 secondes. Au déclenchement, il écrase le profil haut des yeux vers le bas pendant environ 120 ms, puis le fait remonter avec une **courbe de rebond**. (La démo fait déjà une version simple : 70 ms de fermeture, 160 ms de réouverture.)
- **Micro-saccades** : l'œil humain ne reste jamais immobile sur une cible ; il fait de minuscules sauts **de 0,5° à 1° toutes les demi-secondes**. Un bruit de Perlin ou un petit décalage discret de la direction du regard dans le shader rend le visage vivant. (La démo ne fait que de grandes saccades.)

Compléments de Claude (de mémoire, à vérifier) :
- les **grands changements de regard** s'accompagnent souvent d'un **clignement**, et les clignements se placent volontiers aux **pauses de la parole** (lien avec [[Plan_KlattVoice]]) ;
- la **tête accompagne les grands regards** (l'œil part d'abord, la tête suit) ;
- la pupille n'est jamais parfaitement fixe : une **légère oscillation** (l'*hippus*) s'ajoute à la réaction à la lumière.

Tout ça relève du component (étape U5), sans coût GPU notable : ce ne sont que des paramètres qui bougent.

### Faut-il plutôt des yeux en mesh « comme tout le monde » ? (question de l'auteur, réponse de Claude)

- **En coût GPU brut, une paupière en mesh est moins chère par pixel**, surtout en gros plan : quelques triangles et un ombrage simple. La coque d'occlusion et la tearline ajoutent de la transparence superposée, mais restent bon marché.
- **En jeu, la différence est négligeable** : quelques microsecondes dans les deux cas, avec le LOD 2D au loin. L'écart ne compte qu'en très gros plan, et la version analytique (U3) le réduit.
- **Économe au sens large, le mesh ne l'est pas ici** :
  - le projet a décidé « pas de skinned mesh » ([[Plan_NPC]]) ; des paupières en mesh voudraient du skinning ou des morphs, ou des coques rigides qui tournent (l'astuce des vieux jeux), avec peu de variété de forme ;
  - une forme de paupière = un mesh à modéliser, contre quelques nombres ici ;
  - il faudrait refaire l'ombre de contact, la ligne humide et l'absence de traversée, que la méthode obtient gratuitement.
- **Recommandation : rester sur la méthode sans mesh.** Le repli possible, si un jour le coût pose problème en gros plan : des coques rigides qui tournent autour du centre du globe, comme LOD intermédiaire.

### Questions pour l'auteur

- ~~Les cils : comment et où les as-tu réalisés ?~~ → réglé (contresens, voir plus haut).
- ~~Le nom du plugin~~ → **`SDFEyes`** (décision de l'auteur, 2026-10-17 ; `ProceduralEyes` refusé). Emplacement : `Plugins/SDFEyes/`.
- ~~À venir : parler du visage en détail~~ → commencé dans [[Plan_Face]] (2026-10-17). Conséquence pour ce plan : **gros plans de dialogue visés** (Skyrim, Deus Ex, Mass Effect), donc l'étape U3 (analytique) est nécessaire, et les sourcils de `Plan_Face` partageront les pilotes des paupières.

### Coût et optimisation (estimations de Claude, 2026-10-17, non mesurées)

- **La démo ray-marche tout** : de l'ordre de 15 000 opérations par pixel d'œil dans le pire cas. En jeu, à distance normale (≈ 20 × 10 px par œil), c'est négligeable. En très gros plan, un œil en plein écran coûterait **plusieurs millisecondes** : trop cher pour une cinématique sans optimisation.
- **Optimisation principale : passer de la marche à l'analytique.** Le globe est une sphère et la paupière est une coque sphérique découpée par des profils. On intersecte donc directement le rayon avec deux sphères, puis on lit les profils au point d'impact. Seul le bord arrondi demande quelques pas de marche. Gain attendu : de l'ordre de 5 à 10 fois.
- **Ombre et AO analytiques** : rayon vers le soleil → sortie de la coque → « dans la paupière ? » + distance au bord pour la pénombre. L'AO de contact se déduit de la distance au bord, sans sondes.
- **LOD** : loin, pas de cils en relief (peints), pas de veines ; très loin, version 2D.
- Constantes calculées une fois par personnage sur le CPU (comme dans la démo) et passées en Custom Primitive Data. Moins d'`atan` et d'`asin` (comparer des sinus).
- La marche reste utile comme **référence et outil de debug**.
- **Mesure de l'auteur** (2026-10-17, bouton « Mesurer », même carte) : **0,4 ms par image à 810 × 608** (0,5 Mpx).
- **Première mesure** (2026-10-17, démo v2 en ray-march complet, navigateur intégré, **RTX 5070**) : **0,8 ms par image à 611 × 458** (0,3 Mpx, dont environ un tiers de pixels d'œil). Extrapolation grossière : ~7 ns par pixel d'œil. Œil de gameplay (2 × 200 px) : quelques µs. Dialogue (2 × 300 × 150 px) : ~0,6 ms. Œil plein écran 1080p : ~14 ms, d'où l'optimisation analytique pour les gros plans. La démo a maintenant un bouton « Mesurer » et un plein écran à résolution native pour mesurer sur d'autres machines.
- Remarque de l'auteur : la démo tourne en HTML. Précision : WebGL passe par ANGLE vers Direct3D 11, donc c'est **le même GPU et le même genre de shader** qu'en jeu. Le navigateur n'est pas un frein, mais la démo a toute la carte pour elle seule, alors qu'en jeu l'œil partage la frame.

### Pourquoi tout le monde ajoute des meshes (lecture de Claude)

MetaHuman et les démos techniques ajoutent une coque d'occlusion et une tearline à des paupières en mesh. Leurs contraintes ne sont pas celles du projet :
- **la paupière doit suivre le rig facial** (blendshapes, capture 4D, plissements, joues) : un mesh suit le rig sans effort ;
- **pipeline différé + TAA** : il faut une vraie géométrie pour la profondeur, les shadow maps, les vecteurs de mouvement (sinon fantômes en TAA), le SSS, les cils en strands ou en cartes ;
- **silhouette** de profil ;
- **outils d'artistes** (sculpture, scans photoréalistes) plutôt que des paramètres ;
- **coût fixe** : un mesh coûte pareil de loin ou de près, alors que la marche coûte selon les pixels ;
- l'astuce de la coque d'occlusion est **connue et éprouvée** : elle compense ce qu'une paupière en mesh ne fait pas bien, l'ombre de contact douce et la ligne humide.

**DWAM est dans le cas inverse** : forward + MSAA, éclairage maison, pièces rigides sans rig facial ~~, style non photoréaliste~~ (**faux, corrigé par l'auteur le 2026-10-17** : ce n'est pas un style non photoréaliste). Précisions de l'auteur : il pourrait faire un rig ou des drivers pour les yeux, pas de manière classique, et il est son propre artiste. Les raisons de l'industrie ne s'appliquent presque pas, et la méthode obtient gratuitement ce que les autres ajoutent par des meshes, parce que paupière et globe vivent dans le même monde analytique.

### Peau autour de l'œil (question de l'auteur, 2026-10-17)

Possible : cernes, poches, rides. La texture du mesh peut être reprojetée sur la paupière (on connaît le point de départ du rayon dans les UV). Les profils donnent une **direction naturelle des rides** : les lignes parallèles aux bords (pli, plissement de la paupière inférieure), et des rayons partant du coin externe pour les pattes d'oie. Les poches sont un renflement SDF sous la paupière inférieure.

### Ce qui existe déjà (recherche du 2026-10-17)

- **Aucune méthode trouvée de paupières sans mesh, ray-marchées, dans un moteur de jeu.** Le plus proche : *Selfie Girl* d'Inigo Quilez (un visage entier en SDF sur Shadertoy, une œuvre, pas un système) et des paupières toon pilotées par matériau dans Blender (rendu hors temps réel ; instables avec des cils, à cause d'un booléen).
- **Ce que fait l'industrie** (MetaHuman, Character Creator) : des meshes en plus. Une coque transparente « eye occlusion » pour l'ombre et l'AO des paupières sur le globe, et une « tearline » pour l'humidité et la jonction œil / paupière. Ça confirme que **l'ombre des paupières et la ligne humide** sont les deux indices de réalisme clés.
- **Problèmes connus** : la paupière traverse le globe ou la bosse de la cornée pendant un clignement (meshes) ; la jonction entre paupière et œil est trop nette (il faut l'adoucir) ; les cils perturbent les masques ; **la forme de la paupière dépend du regard** (fermeture à peu près linéaire avec le regard vertical, confirmée par un article de Disney Research de 2024 et par RIT-Eyes).
- **Optimisations relevées** : nombre de pas calculé à partir du point d'entrée et de l'épaisseur du volume ; 10 à 20 pas maximum ; code dans des `.ush` ; repli quand les pas sont épuisés.
- À retenir pour le système : la coque doit passer **au-dessus de la bosse de la cornée** de ton prototype (`CorneaBump`), sinon la cornée traverse la paupière.

Sources : [Selfie Girl (MetaFilter)](https://metafilter.com/194834/Paint-the-universe-with-geometry-smoothstep-and-noise) · [paupières toon Blender](https://www.blendernation.com/2023/04/28/procedural-eyelids-for-stylized-cartoon-eyes-in-blender/) · [Eye Occlusion (Reallusion)](https://manual.reallusion.com/Character-Creator-4/Content/ENU/4.0/15_Digital_Human_Shader/Eyes/Digital-Human-Eye-Occlusion-Shader.htm) · [Tearline (Reallusion)](https://manual.reallusion.com/character-creator-4/Content/ENU/4.0/15_Digital_Human_Shader/Eyes/Digital-Human-Tearline-Shader.htm) · [efficient character rendering in UE4 (80.lv)](https://80.lv/articles/efficient-character-rendering-in-ue4/?amp=1) · [Animatable Eyelid Shapes (Disney Research, 2024)](https://arxiv.org/html/2410.01360v1) · [RIT-Eyes](https://arxiv.org/pdf/2006.03642) · [contour de paupière en Bézier](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC12132956/)

## Questions ouvertes

- ~~A ou B~~ → **A d'abord** (auteur, 2026-10-17), puis la méthode en pivot.
- Style : plutôt anime (aplats, trait de bord) ou plutôt peint / réaliste ? `À COMPLÉTER`
- Les cils ? `À COMPLÉTER`
- Où vit l'œil sur un PNJ en pièces rigides : une pièce « visage » qui porte la sphère de l'œil ? `À COMPLÉTER`
