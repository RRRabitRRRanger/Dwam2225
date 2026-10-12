# Plan — Confort d'usage et debug minimal des plugins

> Accueil : [[00 - Accueil]] · Lié à : [[Guide_Watershed]] · [[Guide_Rope]] · [[Guide_LayoutKit]] · [[Guide_Lights]]

> Statut : **proposition, en attente de validation**. Demande de l'auteur du 2026-10-12, après ses premiers tests de Watershed, de la corde et du LOA : « revoir les debug minimaux à faire, surtout en C++ ». Pas de code.

## Ce que l'auteur a demandé (2026-10-12)

1. **Une notice / légende dans un tiroir**, « comme ce que je fais pour mes artistes ».
2. **Des éléments de debug cliquables dans le viewport**, avec des options au clic droit : « des handles en quelque sorte ».
3. **Du texte d'information dessiné au-dessus de tout.**
4. **Les réglages du plugin en haut du panneau Détails**, pas tout en bas (« une drôle de manière de faire »).
5. Et les quatre points proposés par Claude, acceptés (« oui tout ça ») : savoir si le système tourne ; voir pourquoi il ne fait rien ; des boutons de test dans les Détails ; un affichage dans le viewport activable d'une case.

## Pourquoi les réglages sont en bas aujourd'hui

Ce n'était pas un choix : la racine du `Watershed Volume` est une boîte, et Unreal liste d'abord les catégories des composants (*Transform*, *Static Mesh*, *Materials*, *Shape*…), puis celles de la classe. Deux réglages de classe corrigent ça (vérifiés dans le source 5.8, `Material.h:447` s'en sert) :

- `PrioritizeCategories = ("Watershed")` : la catégorie du plugin passe en tête ;
- `HideCategories = (...)` : les catégories sans intérêt disparaissent (*Static Mesh*, *Materials*, *HLOD*, *Mobile*, *Ray Tracing*…).

## La convention proposée, pour chaque acteur ou composant d'un plugin

| # | Élément | Comment (Unreal) |
|---|---|---|
| 1 | **Catégorie du plugin en tête**, le bruit caché | `PrioritizeCategories`, `HideCategories` sur la `UCLASS` |
| 2 | **Notice** repliable en haut des Détails : à quoi sert l'objet, les trois gestes de base, la légende des couleurs du debug | Personnalisation du panneau Détails (`IDetailCustomization`), dans un module d'éditeur |
| 3 | **État** en lecture seule : tourne ou non, chiffres clés (cellules, lacs, nœuds), durée du dernier calcul | Propriétés `VisibleAnywhere`, `Transient` |
| 4 | **Problèmes** : la liste de ce qui empêche le système de marcher, en clair (« la boîte ne touche aucun sol », « aucun matériau ») | Tableau de textes comme le *Problems* de LayoutKit, plus un avertissement dans le viewport |
| 5 | **Boutons de test** en tête de catégorie | `CallInEditor` (déjà en place sur Watershed et LayoutKit) |
| 6 | **Poignées cliquables** dans le viewport, avec **menu au clic droit** | *Component Visualizer* (`FComponentVisualizer`) et ses *hit proxies* ; menu par `GenerateContextMenu` |
| 7 | **Texte au-dessus de tout**, près des poignées | `DrawVisualizationHUD` du visualizer ; les traits en `SDPG_Foreground` |
| 8 | **Debug en jeu** activable sans recompiler | Une variable console par plugin (par exemple `Watershed.Debug 1`) et une catégorie de log |

Le mot que l'auteur cherchait pour les points 6 et 7 : un **Component Visualizer**. `DwamLightingEditor` en a déjà un (les gizmos des lumières) ; il servira de modèle.

## Où mettre ce code

Les points 2, 6 et 7 sont du code d'éditeur : ils ne doivent pas partir dans le jeu packagé. Deux façons :

- **A. Un module d'éditeur par plugin** (`WatershedEditor`, `RopeEditor`…), comme `DwamLightingEditor`. Chaque plugin reste autonome et réutilisable seul. Du code répété.
- **B. Un petit plugin commun** (nom neutre, à choisir) qui fournit la notice, la base des poignées et le texte ; chaque plugin y branche son contenu. Moins de code, une seule apparence partout, mais une dépendance de plus pour chaque plugin.

Avis de Claude : **A pour commencer**, sur un seul plugin, puis extraire le commun vers B quand on aura vu ce qui se répète vraiment.

## Étapes

1. **Pilote sur Watershed** : catégorie en tête et bruit caché ; notice ; état et problèmes ; poignées sur les sources et les lacs (clic droit : ajouter de l'eau ici, vider ce lac, ajouter une source) ; texte au-dessus de tout ; `Watershed.Debug`.
2. Test par l'auteur, corrections.
3. Même traitement pour `Rope`, `LayoutKit` (où il rejoint l'interface d'édition demandée le même jour, voir [[Plan_KitbashTool]]), puis `PatchTerrain`, `GPUTrace`, `Dwam Light`.
4. Extraction du commun (option B) si elle se justifie.

Le point 1 de la convention (catégorie en tête) est une ligne par classe : il peut être appliqué à tous les plugins dès l'étape 1.

## Questions à l'auteur

1. **La notice pour tes artistes** : tu la fais comment aujourd'hui (une catégorie avec un texte, un widget, un commentaire dans le Blueprint) ? Pour reprendre la même forme.
2. **A ou B** pour ranger le code d'éditeur ?
3. **Pilote sur Watershed**, ou tu préfères commencer par un autre plugin ?
4. Au clic droit sur un lac ou une source, quelles actions veux-tu en premier ?
