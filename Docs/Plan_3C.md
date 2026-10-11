# Plan — 3C : personnage, caméra, contrôles

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_NiagaraRope]] (corde accrochée à la main), [[Plan_GpuTraces]]

> Statut : **proposition** (2026-10-04), en attente de validation. Rien n'est codé.

## Intention (auteur, 2026-10-04)

- Un jeu en **vue subjective** où la souris ou la manette **contrôle la main** du personnage.
- **Du contrôle, pas de physique subie** : la référence est *For Honor* (Art of Battle). Pas d'active ragdoll : ni *Octodad* ni *Gang Beasts*.
- Sur PC, **la même souris contrôle la main et la caméra** : il faut un filtrage différent pour chacune, et qui change selon la **stance**.
- **Stances** (il y en aura, c'est certain) : **idle, interact, attack, defence, parade**.
- **Caméra** : pouvoir passer de la vue subjective à une vue épaule, et à autre chose (cinématique, ou un besoin pas encore connu).
- **Un outil visuel pour voir les filtrages.** L'auteur le dit : « je suis humain, je ne lis pas la matrice ». Référence visuelle : la chaîne YouTube *EternalDahaka*, qui analyse les zones mortes, les angles et les courbes des sticks dans les jeux avec des diagrammes en disque. Précision de l'auteur (2026-10-11) : il analyse des jeux sortis ; ce n'est pas une source d'implémentations, de tutoriels ni de publications, donc pas de lien à ranger ici.

## L'existant (lu par les noms des assets, pas par les branchements)

Dans `Content/RPG/Characters/p1/`, à partir du template First Person :

| Élément | Où | Idée |
|---|---|---|
| Filtrage de la souris et du stick | `BP_FirstPersonCharacter_AVO` | Courbes X / Y, sensibilité, lissage, zone de caméra (`Dead Zone Camera`) |
| Visualisation | `WBP_StickVisu`, `WBP_CameraBorders` | Stick brut face au stick filtré, anneaux de zone morte ; bords de la zone de caméra |
| Main | `BP_FirstPersonCharacter_AVO`, `CR_ArmMannequin`, `IK_Arm_Simple` | Cible de la main : projection de l'écran → trace → interpolation, puis IK |
| Geste circulaire | `BP_FirstPersonCharacter_AVO` | Fenêtre glissante, barycentre, rayon moyen, angle cumulé. **Ne marche pas** (auteur) |
| Gravité vers un centre | `BP_FirstPersonCharacter_AVO` | `SetGravityDirection` |
| Entrées | `IMC_CHAR`, Player Controller | Clavier, manette |
| Jauges, acoustique | `Comp_Gauges`, `BPC_Acoustic` | Déjà séparés en composants |

Avis de l'auteur : « ça marche, mais c'est très junky ; à part l'EUW, le système n'est pas scalable ». **On reprend les idées, pas le code.** Le tactile (`Content/Input/Touch/`) vient du template : hors sujet.

## 1. Entrées : Enhanced Input + routeur + sondes

### Ce qu'Enhanced Input fait déjà (vérifié dans le source 5.8)

- Les **modifiers** forment une chaîne de filtres par action : zone morte, échelle, inversion, échange d'axes, lissage, courbes de réponse, mise à l'échelle par le FOV. On peut écrire les siens.
- **Une même source peut alimenter deux actions** : la souris (ou le stick) est mappée sur `IA_Hand` **et** `IA_Camera`, chacune avec **sa propre chaîne**.
- **Une stance = un Input Mapping Context** (ou un ensemble) qu'on active et désactive, avec des priorités. Pas de code.

### Ses limites pour ce jeu (vérifiées dans `InputModifiers.h`)

- **Courbes de réponse par axe** (X, Y séparés), pas sur l'amplitude radiale : une diagonale ne répond pas comme un axe.
- **Pas d'aimant d'angle** (secteurs), indispensable pour une garde directionnelle.
- **Lissage** (`Smooth`) : une moyenne sur un temps très court (~8 ms), non réglable.
- **Pas de partage avec état** entre deux actions : « ce que la main n'absorbe pas va à la caméra » demande de connaître la position de la main. Chaque chaîne est indépendante.
- **Pas de visualisation** étape par étape. `showdebug enhancedinput` affiche les valeurs en texte.

### Proposition

```
Matériel → Enhanced Input (chaînes de modifiers par action) → Routeur (selon la stance) → Main / Caméra
                         ↑ sondes à n'importe quel endroit → outil visuel
```

- **Nouveaux modifiers** (C++, réglages dans l'éditeur, chacun **sait se dessiner** dans l'outil) :
  - **courbe de réponse radiale** (sur l'amplitude, en gardant la direction) ;
  - **aimant d'angle** : secteurs, avec hystérésis pour ne pas clignoter entre deux directions ;
  - **lissage en temps** (constante de temps réglable, indépendant des FPS) ;
  - **sensibilité et accélération de la souris** ;
  - **anti-zone morte**, zones axiales ;
  - **sonde** : ne modifie rien, enregistre ce qui passe.
- **Routeur** : un composant sur le personnage. Il reçoit `IA_Hand` et `IA_Camera` et applique la règle de la stance. Exemples : la main absorbe jusqu'au bord de sa zone puis le surplus va à la caméra ; caméra verrouillée ; partage proportionnel. **Chaque règle est une petite classe** choisie dans le profil de stance.
- **Profil par appareil** : la souris donne un **déplacement** (la main avance par incréments), le stick une **position** (la main peut suivre la position du stick dans sa zone). Les deux ne se filtrent pas pareil.

**Pourquoi ça passe à l'échelle** : un nouveau filtre = une classe ; une nouvelle stance = un Mapping Context + un Data Asset ; l'outil affiche tout sans modification.

## 2. L'outil visuel (prioritaire)

### Les vues

| Vue | Montre |
|---|---|
| **Disque du stick** | Les zones de la chaîne dessinées par les filtres eux-mêmes (zone morte, bandes des axes, secteurs, anneau extérieur), le point brut, le point filtré, leur traînée |
| **Grille déformée** | Une grille régulière passée dans la chaîne : on voit où chaque position atterrit (écrasé, étiré, aimanté) |
| **Courbe de réponse** | Amplitude d'entrée → amplitude de sortie, avec le point actuel |
| **Oscilloscope** | Brut et filtré dans le temps : retard, lissage, à-coups |
| **Souris** | Les mêmes vues en vitesse : sensibilité, accélération |

### Où

- **En jeu**, par-dessus l'image : juger le ressenti en jouant.
- **Dans l'éditeur** (Editor Utility Widget) : entrées synthétiques (cercles, balayages, diagonales).
- **Enregistrement et relecture** : on enregistre quelques secondes d'entrées brutes, on change un réglage, on rejoue **exactement les mêmes gestes** et on compare.

Technique : un widget C++ qui dessine lui-même (`NativePaint`), utilisable en UMG comme dans un EUW. Les filtres sans état sont évalués directement pour la grille ; ceux avec état (lissage) sont remis à zéro pour l'échantillonnage, ou montrés seulement dans l'oscilloscope.

## 3. Caméra par modes

- **Principe** : un **mode** calcule la vue à chaque frame (position, rotation, FOV). Un **directeur**, dans le Player Camera Manager, choisit le mode actif et fait le **fondu** entre deux modes (durée, courbe). Les modes s'empilent par priorité, une cinématique pouvant passer par-dessus.
- **Modes de départ** : **subjectif** (tête, piloté par le routeur), **épaule** (décalage, bras de caméra avec collision), **externe** (une Cine Camera, une spline ou le Sequencer prend la main).
- **Pourquoi un système maison** :
  - **Lyra** est un *projet d'exemple* d'Epic, pas une fonction du moteur.
  - **Gameplay Cameras** est un plugin du moteur, encore **expérimental (v0.1) en 5.8** (vérifié dans son `.uplugin`).

  Le principe est simple : l'auteur le comprendra en entier, et on pourra migrer plus tard si le plugin mûrit.
- **Vue subjective** : garder le rendu « première personne » d'Unreal 5.x (FOV et échelle séparés pour les bras, déjà utilisés dans le Blueprint actuel).

## 4. La main

- Une **cible dans l'espace de la vue** (position 2D dans une zone, plus une profondeur), pilotée par `IA_Hand` après le routeur. Elle revient au repos, avec des limites par stance. Une trace la projette dans le monde quand il faut (interagir).
- **IK** : la Control Rig de l'auteur (`CR_ArmMannequin`).
- **Garde directionnelle** : position de la main → direction (secteurs, avec hystérésis).
- **Gestes** (cercle et autres) : reconnus sur la **trajectoire de la main** filtrée, pas sur la souris brute. Une fenêtre **en temps**, pas en images. Méthodes candidates :
  - cumuler la rotation de la direction du mouvement : une boucle fait ±360°, quel que soit le centre ;
  - le « $1 Unistroke Recognizer » (Wobbrock et al., 2007) pour plusieurs formes.

  Causes probables de l'échec actuel (sans avoir vu le code) : barycentre biaisé sur un arc incomplet, angle qui saute à ±180°, fenêtre en images.

## 5. Les stances

- **idle, interact, attack, defence, parade.**
- Chaque stance = un **Data Asset** : Mapping Contexts à activer, règle du routeur, mode de caméra, règles de la main (zone, repos), transitions permises.
- **Machine à états** : un enum et des transitions en C++ pour commencer ; un State Tree si les transitions deviennent riches.

## 6. Déplacements (souhaits de l'auteur, 2026-10-04)

| Souhait | Référence | Piste technique |
|---|---|---|
| Marcher, courir, s'accroupir | — | Natif dans le Character Movement (accroupi compris) |
| **Glissade** : courir puis s'accroupir = glisser, avec un peu de contrôle | — | Mode de déplacement maison : élan conservé, direction un peu orientable, la pente accélère ou freine |
| **Coyote time** | — | Saut permis quelques instants après avoir quitté le sol. À coupler avec un **tampon de saut** (saut pressé juste avant d'atterrir) |
| **Sauter sur les ennemis** | *Mario* | Atterrir sur un ennemi par le dessus : rebond + effet (dégâts, étourdissement), avec une tolérance |
| **Escalader** | *Dragon's Dogma* | Mode « escalade » maison. Dragon's Dogma permet aussi de grimper **sur les grands monstres** (surfaces mobiles) : périmètre à préciser |
| **Pousser l'objet sous ses pieds en sautant** | Un proto Unity de l'auteur | Au saut, une impulsion opposée sur l'objet sous les pieds, s'il porte un composant (ou un tag) « poussable » et qu'il est physique |
| **Lever le pied** : ne jamais bloquer sur un obstacle de 20 cm (Skyrim le fait) | Le *Step Offset* du Character Controller d'Unity | Unreal a l'équivalent (`MaxStepHeight`, 45 cm par défaut), mais fixe. Le rendre **dynamique** (selon la vitesse, la stance), enjamber au-delà (genou, hanche), IK des pieds pour que le pied se lève vraiment |
| **Inertie dans les pentes** : en courant dans une pente forte, on est emporté ; s'arrêter demande un effort, « comme sprinter en arrière » | — | Accélération dans le sens de la pente, freinage réduit au-delà d'un angle. Classes de pente : marchable, avec élan, glissade, infranchissable |
| **Outil : voir les endroits qui mettent dans cette situation** | — | Carte des pentes colorée par classe (autour du joueur ou sur tout le niveau) ; plus tard, les zones où l'élan emmène vers un vide |

**Base** : le Character Movement d'Unreal, étendu par des modes maison (glissade, escalade). Le plugin *Mover*, son successeur, est encore **expérimental en 5.8** (vérifié dans son `.uplugin`).

**Décisions de l'auteur (2026-10-04)** :
- **Tampon de saut** : oui, avec le coyote time.
- **Escalade** : le décor **et les grands monstres**, comme dans Dragon's Dogma.
- **Pas de Mover**.
- **Enjamber** : sur une touche, mais **pas un QTE**. C'est la touche de sprint : l'obstacle est devant, j'avance en sprintant, et ça passe.
- **La main pendant les déplacements** : par défaut elle ne fait rien, placée comme dans un FPS classique, avec un peu d'animation (références : *Overwatch*). Plus tard, peut-être : s'agripper à quelque chose pendant une glissade (« on n'en est pas là du tout »).

### Anti-coincement « code Konami » (idée de l'auteur)

Constat : on reste bloqué dans un coin, un petit trou, parfois à cause d'un seul sommet de la map, et le joueur martèle les touches dans tous les sens.

Idée : **détecter ce martèlement comme un code** (beaucoup d'entrées, de directions qui changent, presque aucun déplacement pendant un moment). Le jeu dégage alors le joueur, avec un **petit bonus** en clin d'œil.

Pistes techniques : comparer l'énergie des entrées (amplitude, changements de direction) au déplacement réel sur une fenêtre de temps. Pour dégager le joueur : chercher la position libre la plus proche (balayages autour), un petit coup de pouce vers le haut, ou un retour à la dernière position sûre.

Questions ouvertes :
- Escalade : avec une endurance ? Comment on s'accroche (touche, contact) ?
- Sauter sur un ennemi : rebond seulement, dégâts, ou ça dépend de l'ennemi ?
- Glissade : quelle part de contrôle (orienter, freiner, sauter en sortie) ?
- Lever le pied : jusqu'à quelle hauteur sans animation ?
- Anti-coincement : quel genre de petit bonus ?

## 7. Combat (auteur, 2026-10-04)

- **L'attaque vient du mouvement** : direction et amplitude. La souris va d'un point A à un point B, la main se déplace **et tourne** en conséquence.
- **Lisibilité** : marquer l'**armé** (windup) et les autres phases du coup, avec les principes classiques de l'animation (anticipation, action, retour).
- **Parade** : on bouge beaucoup moins, mais on **tourne le poignet** pour intercepter l'attaque. Pas encore testé par l'auteur.
- **Contact de l'arme** : un coup peut **rebondir**, ou **glisser** contre l'objet ou l'arme touchés. C'est une réponse au contact (selon l'angle, la force, la matière), pas une simulation physique libre : on garde le contrôle (pas d'active ragdoll).
- **Archétypes de jeu de combat** (rushdown, zoner, grappler…) : ce sont des **classes de joueur**. Le joueur est poussé à en choisir une ; sinon, il reste **all-rounder**. Ça implique aussi les mécaniques des jeux de combat (phases d'un coup : démarrage, actif, récupération ; étourdissement ; contres ; mix-ups ; jeu à la limite de portée).
- **Équilibre** : un pierre-feuille-ciseaux **gigogne** (version poussée de *Destrega*), avec les archétypes à la base ; voir [[Plan_Loot]], section « Équilibrage ».
- **Archétype et arme sont indépendants** (validé par l'auteur, 2026-10-04). L'**archétype** est le plan de jeu (gestion de la distance et du tempo) : techniques, options de déplacement, stances, profils des jauges. L'**arme** donne les outils et la maniabilité : portée, masse, inertie (le filtre du geste), réaction au contact. Une classe peut **favoriser** des armes sans les imposer. Exemples : un rushdown à la lance (main avancée sur la hampe, estocs enchaînés, talon de la hampe au contact, coincer l'adversaire) ; un zoner à la dague (lancers, pièges et zones interdites comme l'huile enflammée, jeu de jambes, mortel au contact).
- **Appareils** : l'auteur joue au **clavier / souris** ; la **manette** est nécessaire aussi.

## 8. Interactions (auteur, 2026-10-04)

- **Attraper, lancer, pousser, tirer.**
- **Actionner des mécanismes**, au minimum comme dans *Zack & Wiki* : tourner, tirer, abaisser, avec le geste.
- **Utiliser un objet de la scène comme bras de levier.**
- **Emotes** : un petit menu, très fluide et instinctif. Pas le cœur du jeu.

## 9. Gestes et formes (auteur, 2026-10-04)

- **Reconnaissance de formes** : oui (cercle et autres). L'auteur n'a pas réussi à mettre en place le « $1 Recognizer ».
- **Selon l'arme** : avec une épée, faire un cercle est difficile, comme un vrai coup horizontal ; avec une lance légère, on peut faire des cercles et des formes plus lisses.
- **Tracer**, comme dans *Skyward Sword* (dessiner un motif avec l'arme), avec le même principe.

**Proposition (Claude) : l'arme est un filtre.** La maniabilité d'une arme (inertie, vitesse de rotation maximale) filtre la trajectoire de la main, comme un filtre de la chaîne d'entrée. On reconnaît les formes sur la **trajectoire de la pointe de l'arme**, pas sur la souris. Une épée lourde « refuse » alors naturellement un petit cercle rapide, une lance légère le permet, sans règle spéciale par arme. Le tracé à la *Skyward Sword* réutilise la même trajectoire.

**Le « $1 Recognizer » en clair** (Wobbrock, Wilson & Li, 2007). On compare un tracé à des modèles enregistrés :
1. **Rééchantillonner** le tracé en N points régulièrement espacés (ex. 64), quelle que soit la vitesse du geste.
2. **Tourner** le tracé pour que l'angle du premier point au centre soit 0. Option à désactiver quand la direction compte (une entaille horizontale n'est pas verticale).
3. **Mettre à l'échelle** dans un carré et centrer à l'origine.
4. **Comparer** à chaque modèle préparé de la même façon : distance moyenne entre points correspondants (avec un petit ajustement de rotation). Le plus proche gagne, avec un score de confiance.

Limites : il faut savoir où commence et finit le geste (découpage par les pauses ou la vitesse) ; un cercle qui continue (moulinet) se détecte mieux par la rotation cumulée de la direction (§ 4).

## Répartition

| Qui | Quoi |
|---|---|
| **Claude** | Modifiers, sondes, routeur, outil visuel (C++), système de modes de caméra, cible de la main, machine des stances, reconnaissance de gestes |
| **Auteur** | Input Actions et Mapping Contexts, Data Assets des stances, réglages, Control Rig, animations, tests de ressenti |

**Noms** : le filtrage, l'outil et les modes de caméra sont génériques, donc des plugins sans « Dwam » (ex. `InputFiltering`, `CameraModes`). La main et les stances sont propres au jeu, donc dans le module `Dwam2225`.

## Étapes proposées

1. **Sondes + outil visuel** (disque, oscilloscope) sur les actions actuelles : voir tout de suite le filtrage existant.
2. **Nouveaux modifiers** (courbe radiale, aimant d'angle, lissage en temps, souris), chacun dessiné dans l'outil ; grille déformée ; enregistrement et relecture.
3. **Routeur + main** (profils souris et manette), stances idle et interact.
4. **Modes de caméra** (subjectif, épaule, externe) avec fondu.
5. **Stances attack, defence, parade** : garde directionnelle, gestes.

## Questions ouvertes

**À trancher tôt** (elles décident de l'architecture) :
1. Ce que fait chaque stance avec la main et la caméra (en défense : caméra verrouillée sur la cible, comme dans *For Honor* ?).
2. ~~Appareil prioritaire~~ → **clavier / souris d'abord, manette aussi** (auteur, 2026-10-04).
3. ~~L'attaque : mouvement ou bouton ?~~ → **le mouvement** (direction, amplitude), voir § 7.
4. Grimper sur les monstres : décidé (oui), donc le déplacement doit gérer les surfaces mobiles dès le départ.
5. ~~Les archétypes : à quoi s'appliquent-ils ?~~ → **des classes de joueur** (choisir, sinon all-rounder), avec les mécaniques des jeux de combat (auteur, 2026-10-04).

**À régler en jouant** : en vue épaule, la souris contrôle-t-elle encore la main ? L'outil dans l'éditeur (EUW) ? La gravité vers un centre ? Et les questions de détail du § 6.
