# Plan — Fiches de personnage des PNJ

> Accueil : [[00 - Accueil]] · Lié à : [[Guide_StateTree]], [[Plan_EireneNexus]], [[Plan_VitalPotential]], [[Plan_3C]], [[Plan_Gauges]]

> Statut : 💭 **idée**, en discussion avec l'auteur (2026-10-04). Rien n'est codé.

## Intention (auteur, 2026-10-04)

- **Des fiches de personnage pour les PNJ.**
- **Déformer le squelette** pour obtenir beaucoup de silhouettes différentes. Et, tant qu'à faire, **pouvoir faire un cheval** avec le même système.
- **La logique** aussi : champ de vision, type d'aggro, etc.

## L'existant (lu par les noms des assets)

Dans `Content/AVOLABO/New_AI/` :
- `PDA_ProfileSheet_AgentLogic` (Primary Data Asset) : `Sight Radius`, `Lose Sight Radius`, `Pursuit Duration`, `Intelligence Tier`, perception du contrôleur. L'infobulle de `Intelligence Tier` annonce la suite : **« théorie de l'esprit, persistance des objets, sociabilité »**.
- `PDA_ProfileSheet_AgentVisual` : vide pour l'instant.
- `S_BaseAI__Sheet_Redone` (struct), `STT_ReserveSlot` (slots de combat), `STEval_TargetingFocus`.

L'intention de séparer la **logique** du **visuel** est déjà là.

## Proposition (Claude) : une fiche = des modules

Une fiche est un assemblage de modules, chacun un Data Asset réutilisable. Un « type » de PNJ est un **modèle** ; chaque individu est tiré de ce modèle avec des **variations** (des plages au lieu de valeurs fixes).

| Module | Contenu |
|---|---|
| **Corps** | Le profil de déformation du squelette (voir plus bas), la taille, la masse |
| **Perception** | Vision : portée, **angle du champ de vision**, vision périphérique, perte de vue, mémoire de la dernière position. Ouïe (le composant acoustique `BPC_Acoustic` existe déjà) |
| **Tempérament / aggro** | Type d'aggro : territorial, embusqué, curieux, lâche, en meute, protecteur… Seuils de fuite, de poursuite (`Pursuit Duration`) |
| **Combat** | Archétype (rushdown, zoner, grappler, all-rounder), distance préférée, armes ([[Plan_3C]] § 7) |
| **Esprit** | Le niveau d'intelligence (`Intelligence Tier`) : mémoire, théorie de l'esprit, persistance des objets, sociabilité. Seuls certains PNJ ont une mémoire ([[Plan_EireneNexus]]) |
| **Valeurs** | Les tags de goûts et de dégoûts d'Eiréné Nexus |
| **Jauges** | Vie, équilibre, endurance ([[Plan_Gauges]]) |
| **Potentiel vital** | Ce que le PNJ porte, et ce qu'il rend à sa mort ([[Plan_VitalPotential]]) |

Le State Tree lit la fiche : un évaluateur expose les valeurs, les tâches s'y relient ([[Guide_StateTree]]). Un même arbre sert alors à beaucoup de PNJ différents.

## Déformer le squelette

### Les approches dans Unreal 5.8

| Approche | Principe | Pour / contre |
|---|---|---|
| **Échelle et longueur des os** | Allonger ou épaissir des os (jambes, buste, cou, tête) dans l'AnimBP ou la Control Rig, à partir de la fiche | Simple, à l'exécution, marche sur **n'importe quel squelette**, cheval compris. Mais une jambe plus longue fait glisser les pieds : il faut de l'**IK des pieds** et adapter la longueur des pas |
| **Morph targets** | Des formes sculptées (gros, musclé, maigre, visage) mélangées par des poids | Beau et précis, mais chaque forme doit être sculptée |
| **Mutable** | Le système d'Epic de personnages personnalisables, avec *Mutable Population* pour générer des foules | Très complet. Mutable est en **bêta** en 5.8, *Mutable Population* **expérimental** (vérifié dans leurs `.uplugin`) |
| **Retargeting (IK Rig)** | Adapter les animations à des proportions différentes | Stable en 5.8. Utile si les proportions changent beaucoup |

### Proposition

- **Un profil de déformation indépendant du squelette** : des **groupes** (jambes, bras, buste, cou, tête, queue) avec, pour chacun, un facteur de **longueur** et un facteur d'**épaisseur**. Une petite table par squelette relie les groupes aux os réels : le même profil s'applique à un humain ou à un cheval.
- **Allonger par translation le long de l'os** plutôt que par échelle : la peau ne se déforme pas en biais et les enfants ne sont pas écrasés.
- **IK des pieds** et **longueur des pas** adaptées, pour que les animations restent justes.
- Des **morph targets** pour le volume (gros, musclé), plus tard, si besoin.

### Décision de l'auteur (2026-10-04) : un seul squelette, le mannequin

- **Prendre le mannequin d'Unreal et le déformer** pour en faire d'autres bipèdes, et aussi des quadrupèdes (le cheval) : **un squelette réutilisable**. Certains jeux procèdent ainsi.
- **Les animaux** sont des **montures potentielles**, en plus d'être **escaladables** ([[Plan_3C]] § 6).

### Ce que ça implique (Claude)

- **L'anatomie s'y prête.** La patte avant d'un cheval correspond au bras humain : humérus, avant-bras, puis une main très longue (les métacarpiens) sur un seul doigt. Le « genou » avant du cheval est en réalité le poignet. La patte arrière correspond à la jambe : cuisse courte, tibia long, pied très long. Le cheval marche sur la pointe de son unique orteil. Le profil de longueurs suffit donc à passer de l'un à l'autre.
- **Ce qui manque au mannequin** : une queue, des oreilles. Unreal permet d'**ajouter des os** à un squelette partagé (à l'import d'un mesh qui en a davantage). Les meshes qui ne s'en servent pas les ignorent, et les animations existantes continuent de fonctionner. Une queue peut aussi être une petite chaîne simulée ([[Plan_NiagaraRope]], plugin `Rope`).
- ~~**Le mesh reste à faire pour chaque créature.**~~ → remplacé par des pièces rigides (décision ci-dessous).
- *(Avant la décision)* **Le mesh reste à faire pour chaque créature.** Déformer le squelette déplace la peau ; un mannequin étiré à quatre pattes reste un humain étiré. Chaque créature a besoin de son propre mesh, skinné sur le squelette partagé (dans Blender, par exemple). Pour prototyper, des meshes simples suffisent.
- **Les animations** : celles des bipèdes se partagent entre tous les bipèdes. Les quadrupèdes ont besoin des leurs : créées à la main sur le mannequin à quatre pattes, ou **procédurales** (allures, placement des pieds par IK, dans la Control Rig, avec des nœuds C++).
- **Ce qu'un squelette commun apporte** : les outils marchent pour toutes les créatures. Le profil de déformation, l'IK des pieds, les nœuds de Control Rig, et l'**escalade**, puisque les zones où l'on s'agrippe peuvent se définir par des noms d'os standard (cou, colonne, membres) à l'aide du Physics Asset.
- **Monture** : un point d'attache (socket sur la colonne) et l'IK du cavalier (mains sur les rênes, pieds dans les étriers).

### Décision de l'auteur (2026-10-04) : pas de skinned mesh

- **Pas de meshes skinnés** : pas assez de temps. Le **transfert de normales en shader** ([[Plan_NormalTransfer]]) doit permettre de s'en sortir.
- La première créature « bizarre » n'est pas encore choisie.

### Ce que ça implique (Claude)

- **Des pièces rigides sur le squelette** : chaque partie du corps est un mesh statique (ou une forme simple) attaché à un os du mannequin. Le squelette sert seulement d'armature animée : un mannequin invisible qui continue de calculer sa pose.
- **La déformation devient naturelle** : allonger un groupe d'os déplace les pièces, l'épaisseur se règle par l'échelle de la pièce.
- **Le transfert de normales cache les jointures**, comme pour le snake (`Snake Toy`), qui est déjà une chaîne de pièces rigides lissée par le shader.
- **Attention au retard du GDF** : la variante du transfert qui lit le Global Distance Field retarde sur les objets qui bougent (la « reptation » documentée dans [[Plan_NormalTransfer]]). Un personnage bouge tout le temps. Le snake évite ce problème en passant à chaque pièce ses voisines (positions et rayons dans les Custom Primitive Data) : c'est la piste pour les créatures.
- **Ce que les pièces rigides apportent en plus** :
  - créer une créature = assembler des pièces (kitbash) ;
  - une pièce peut **se briser** ou tomber, comme une arme ([[Plan_Loot]]) ;
  - chaque pièce a sa collision, ce qui sert à l'**escalade**.
  - **S'accrocher à un PNJ qui bouge devient simple** (validé par l'auteur, 2026-10-17, « excellent ») : un os + un offset local dans la pièce rigide, au lieu de suivre un point sur une peau déformée. C'est la « brique A » du doc « Plan de jeu UE5 » (grimper, chevaucher, planter une arme), présentée là comme l'inconnue la plus risquée ; avec des pièces rigides, elle l'est beaucoup moins.
- **Plus tard, si le nombre de pièces coûte cher** : regrouper les pièces identiques en instances (un seul appel de rendu par type de pièce).

## Navigation (auteur, 2026-10-04)

- **Navigation Invokers** : le NavMesh n'est généré qu'autour des acteurs qui portent un *Navigation Invoker* (le joueur, les PNJ actifs), en génération dynamique. C'est la réponse au terrain procédural et aux grandes cartes.
- À vérifier au montage : le terrain PatchTerrain (Dynamic Mesh) doit **affecter la navigation** (collision activée, *Can Ever Affect Navigation*).

## Règle de capacité (doc de l'auteur, 2026-10-16)

- **Chaque capacité porte qui peut l'exécuter et qui peut la subir**, avec un défaut par capacité (réservée au joueur ou non). À poser dès la fondation.
- **Interactions PNJ vers PNJ** : composants génériques dès le départ, jamais codés pour le joueur seul. Un PNJ peut grimper sur un autre, le monter, etc.
- Restent à définir : les règles par défaut de chaque capacité, et la forme du bus d'événements « X affecte Y » qui les porte.

## Questions ouvertes

- **Jusqu'où déformer** : variations naturelles d'une même espèce, ou de vraies créatures différentes ?
- ~~Combien de squelettes~~ → **un seul, le mannequin** (auteur, 2026-10-04).
- ~~Le cheval : monture ?~~ → **les animaux sont des montures potentielles, et escaladables**.
- **Quelles créatures en premier** (cheval, loup, grand monstre à escalader) ?
- **Les types d'aggro** que tu veux en premier.
- **Le visuel des fiches** : en C++ (Data Assets modulaires), ou garder tes Primary Data Assets Blueprint ?
