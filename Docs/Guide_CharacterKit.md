# Guide — CharacterKit : tester l'assemblage (étape M1)

> Accueil : [[00 - Accueil]] · Plan : [[Plan_Mannequin]]

> État (2026-10-21) : plugin `Plugins/CharacterKit/` **compilé** (`UnrealEditor-CharacterKit.dll`). **Rien n'est encore vu dans l'éditeur.** But : des pièces (formes du moteur) qui suivent le mannequin pendant qu'il s'anime.

## Ce que contient le plugin

- **`FCharacterDescription`** : la description d'un personnage, de simples données qu'on copie, modifie et sauvegarde. Pour l'instant, une liste de pièces. Chaque pièce = **Slot** (nom de l'emplacement), **Bone** (os du mannequin), **Mesh**, **Offset** (position, rotation, échelle dans le repère de l'os), **Material** (facultatif).
- **`UCharacterPreset`** (asset) : un **préréglage**, qui contient une description de départ. En jeu, un asset est en lecture seule : le joueur ne le modifie jamais, il modifie une copie.
- **`UCharacterAssemblyComponent`** (component) : à poser sur un acteur qui a un Skeletal Mesh. Il crée une pièce par ligne de l'asset et l'attache à son os. Les pièces sont **recréées à chaque modification** et **ne sont jamais sauvegardées** (elles n'apparaissent pas dans la liste des composants).
  - **Use Preset** : coché = la description vient du préréglage ; décoché = la **Description** propre au component (éditable dans Details) ;
  - **Preset** : l'asset ;
  - **Skeleton Component Name** : vide = le premier Skeletal Mesh de l'acteur ;
  - **Hide Skeleton Mesh** : cache le mannequin pour ne voir que les pièces ;
  - bouton **Assemble** : force la reconstruction ;
  - en Blueprint : **Get Current Description**, **Set Description** (écran du joueur, chargement de sauvegarde), **Set Part Mesh** (changer la pièce d'un emplacement, le futur swap de vêtements ; passe sur la description propre), **Get Part Component**.

## 1. Vérifier le plugin

Menu **Edit → Plugins**, chercher **Character Kit** : coché (activé par défaut).

## 2. Le préréglage

1. Content Browser, dossier de test → clic droit → **Miscellaneous → Data Asset**. Dans la liste, choisir **CharacterPreset**. Nom : `DA_TestPreset`.
2. L'ouvrir. Dans **Parts**, cliquer **+** pour chaque pièce. Pour commencer, quelques pièces avec les formes du moteur (dans le sélecteur de **Mesh**, cocher **Show Engine Content** dans les options de la liste si les formes n'apparaissent pas) :

| Slot | Bone | Mesh | Offset (Location / Rotation / Scale) |
|---|---|---|---|
| `Head` | `head` | `Sphere` | (10, 0, 0) / 0 / (0.22, 0.2, 0.25) |
| `Torso` | `spine_03` | `Cube` | (10, 0, 0) / 0 / (0.35, 0.25, 0.4) |
| `UpperArm_L` | `upperarm_l` | `Cylinder` | (15, 0, 0) / (0, 90, 0) / (0.1, 0.1, 0.3) |
| `LowerArm_L` | `lowerarm_l` | `Cylinder` | (13, 0, 0) / (0, 90, 0) / (0.08, 0.08, 0.26) |
| `Thigh_L` | `thigh_l` | `Cylinder` | (-22, 0, 0) / (0, 90, 0) / (0.14, 0.14, 0.42) |
| `Calf_L` | `calf_l` | `Cylinder` | (-20, 0, 0) / (0, 90, 0) / (0.11, 0.11, 0.4) |

Ces valeurs sont **un point de départ** : le repère des os du mannequin n'est pas le même partout (bras et jambes ont leur axe X le long de l'os, mais dans des sens différents). Si une pièce part de travers, on corrige son **Offset** (voir § 4). Les formes du moteur font 100 unités : une échelle de 0.3 donne 30 cm.

## 3. L'acteur de test

1. Content Browser → clic droit → **Blueprint Class → Actor**. Nom : `BP_TestMannequin`. L'ouvrir.
2. **Add** → **Skeletal Mesh**. Dans **Details** :
   - **Skeletal Mesh Asset** : `SKM_Manny_Simple` (dans `Characters/Mannequins/Meshes`) ;
   - **Animation Mode** : `Use Animation Asset` ; **Anim to Play** : `MF_Unarmed_Walk_Fwd` (dans `Characters/Mannequins/Anims/Unarmed/Walk`), ou `MM_Idle`.
3. **Add** → chercher **Character Assembly** (catégorie Character). Dans **Details** : **Use Preset** coché, **Preset** = `DA_TestPreset`.
4. **Compile**, **Save**. Glisser `BP_TestMannequin` dans le niveau.

## 4. Ce qu'il faut regarder

- Les pièces apparaissent sur les os, **dans le viewport du niveau** (et dans celui du Blueprint).
- **Play** (ou **Simulate**) : le mannequin marche, **les pièces suivent leurs os**.
- Cocher **Hide Skeleton Mesh** : seules les pièces restent, et bougent toujours.
- **Set Part Mesh** en Blueprint (par exemple sur une touche, en Play) : la pièce change, et **Use Preset** se décoche (le component a maintenant sa propre description).
- Modifier une ligne de `DA_TestPreset` (mesh, offset), sauvegarder, puis cliquer **Assemble** sur le component de l'acteur posé : la pièce change.
- **Pour trouver le bon Offset** : ouvrir `SKM_Manny_Simple`, onglet **Skeleton**, sélectionner l'os, regarder ses axes dans le viewport. Le X de l'os pointe-t-il vers l'enfant ou vers le parent ? Le signe de la Location en X en dépend.
- **Un os mal nommé** : la pièce n'apparaît pas et l'**Output Log** affiche `LogCharacterKit: Warning: … os 'xxx' introuvable`.

## Limites connues à ce stade

- Pas encore de profil de déformation (M2), d'ancres ni d'yeux (M3), de lissage des jointures (M4), d'outil (M5).
- Les pièces n'ont pas de collision (coupée exprès pour l'instant).
- Une modification de l'**asset** ne met pas à jour tout seul les acteurs déjà posés : bouton **Assemble**, ou recompiler le Blueprint.
