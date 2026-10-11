# Plan — État du monde et persistance

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_PatchTerrain]], [[Plan_Water]], [[Plan_Fire]], [[Plan_Loot]]

> Statut : 💭 **idée** (2026-10-04). L'auteur n'avait pas encore tranché ; rien n'est décidé.

## La question

Quand le joueur brûle une prairie, verse de l'eau, coupe une branche ou déplace un objet : qu'est-ce qui reste ?

## Options

| Option | Principe | Coût |
|---|---|---|
| **Tout se régénère** | Comme Diablo 2 : on revient, tout est neuf | Le plus simple |
| **Tout persiste** | On sauvegarde les **différences par rapport à la graine** du monde procédural : « zone brûlée », « eau versée », « branche coupée » | La sauvegarde grossit avec le temps |
| **Le monde guérit** | L'herbe repousse, les cicatrices s'effacent ; seul l'important persiste (premiers Far Cry, cités par l'auteur) | Sauvegarde bornée ; le temps qui passe se voit |

L'idée « graine + différences » a plu à l'auteur (2026-10-04).

## Pourquoi c'est compatible avec l'existant

Chaque système est déjà un **état posé sur une base déterministe** :
- **PatchTerrain** : le terrain vient d'une graine.
- **Watershed** : l'état est « l'eau versée par cellule » ; tout le reste se recalcule.
- **Feu** : l'état des nodes (brûlé, date de l'incendie).
- **Loot** : un objet est une graine + ses changements (pièces remplacées, connaissance acquise).

Sauvegarder = graine du monde + liste des différences, chacune avec une date (pour la guérison).

## Questions ouvertes

- Quelle option, éventuellement selon le type de changement (un incendie guérit, un pont détruit reste) ?
- Quelle durée de guérison, et le temps passe-t-il quand le joueur est ailleurs ?
