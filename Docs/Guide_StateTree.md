# Guide — State Tree (référence pour DWAM)

> Accueil : [[00 - Accueil]] · Lié à : [[Plan_3C]], [[Plan_Gauges]], [[Plan_Fire]]

> État (2026-10-04) : écrit à partir du **source d'Unreal 5.8** (plugins `StateTree` et `GameplayStateTree`) et des noms trouvés dans les State Trees de l'auteur. Ce qui n'a pas été vérifié est marqué `À VÉRIFIER`.

## En une phrase

Un State Tree est un **arbre d'états**, à mi-chemin entre la machine à états et le Behavior Tree. À chaque décision, Unreal descend l'arbre depuis la racine pour choisir un état feuille. Les **états actifs** sont tout le chemin de la racine à cette feuille, et **les tâches de tous ces états tournent en même temps**. Les **transitions** font passer d'un chemin à un autre.

## Les briques

| Brique | Rôle | Chez toi |
|---|---|---|
| **État** | Un nœud de l'arbre : des tâches, des conditions d'entrée, des transitions, des enfants | `Idle`, `Chase`, `Attack`, `Flee`, `State_Idle_Wander`… |
| **Tâche** (Task) | Ce qui *se passe* pendant que l'état est actif | `STT_Chase_Target`, `STT_GetPlayer_Redone`, `STT_Wander_Redone`, `STT_switchAction`, `Move To`, `Debug Text` |
| **Condition** | Vrai / faux : peut-on entrer dans l'état, une transition doit-elle partir ? | `ST_C_Distance`, `ST_C_Flee`, `Float Compare` |
| **Évaluateur** (Evaluator) | Calcule des données **une fois par tick, pour tout l'arbre** (cible, distances) | `STEval_Utility_Redone` |
| **Considération** (Consideration) | Un score pour la sélection par **utilité** | `Float Input` + courbe de réponse (`Score_Attack`) |
| **Fonction de propriété** (Property Function) | Calcule une valeur juste avant qu'elle soit transmise à un nœud | — |
| **Tâches globales** | Des tâches qui tournent pendant toute la vie de l'arbre | — |
| **Paramètres** | Des valeurs réglables de l'arbre, ou passées à un sous-arbre lié | — |
| **Contexte** | Ce que le schéma fournit : l'acteur, le contrôleur d'IA | — |

## Types d'état (vérifié, `StateTreeTypes.h`)

| Type | Rôle |
|---|---|
| **State** | Un état avec des tâches et des enfants |
| **Group** | Seulement des enfants (un dossier logique) |
| **Linked** | Renvoie vers un **sous-arbre** du même asset (réutilisation interne) |
| **Linked Asset** | Renvoie vers **un autre State Tree** (réutilisation entre assets) |
| **Subtree** | Un sous-arbre vers lequel on peut renvoyer |

## Comment un état est choisi (vérifié)

Réglage *Selection Behavior* d'un état :

| Valeur | Effet |
|---|---|
| *None* | L'état ne peut pas être choisi directement |
| *Try Enter* | L'état est choisi même s'il a des enfants |
| *Try Select Children In Order* | Le premier enfant qui accepte (conditions remplies), dans l'ordre |
| *Try Select Children At Random* | Les enfants mélangés, puis le premier qui accepte |
| *Try Select Children With Highest Utility* | L'enfant au **meilleur score** (considérations) ; à égalité, dans l'ordre. **C'est ce qu'utilise ton IA.** |
| *Try Select Children At Random Weighted By Utility* | Tirage au sort, chance proportionnelle au score |
| *Try Follow Transitions* | Essaie les transitions de l'état au lieu d'y entrer |

## Transitions (vérifié)

- **Déclencheurs** : à la fin de l'état (*On State Completed*, *Succeeded*, *Failed*), à chaque tick (*On Tick*), sur un **événement** (*On Event*), sur un **délégué** (*On Delegate* : une tâche « diffuse », la transition « écoute »).
- **Destinations** : un état précis, le parent, l'état suivant, le prochain état sélectionnable, ou terminer l'arbre (succès / échec).
- **Priorités** : *Low*, *Normal*, *Medium*, *High*, *Critical*. Si plusieurs transitions partent en même temps, la première de plus haute priorité gagne.
- Chaque transition peut avoir ses **conditions**.

## Quand un état est-il terminé ?

Un état se termine quand ses tâches se terminent (succès ou échec). Une tâche peut être exclue de ce calcul (*Considered For Completion*, réglable par tâche). En Blueprint, une tâche se termine avec **Finish Task** (succès ou échec). Une tâche qui ne se termine jamais garde l'état actif, ce qui est voulu pour « attendre » ou « suivre ».

## Les données : liaisons (bindings)

Les propriétés des nœuds ont une **catégorie** qui décide de leur rôle (vérifié, `StateTreePropertyBindings.cpp`) :

| Catégorie | Rôle |
|---|---|
| **Input** | Doit être reliée à une source (contexte, sortie d'un autre nœud, paramètre) |
| **Output** | Exposée aux autres nœuds (ex. la cible trouvée par `STT_GetPlayer`) |
| **Context** | Reliée automatiquement au contexte (l'acteur, le contrôleur) |
| **Parameter** | Une valeur réglée dans l'arbre, ou reliée |

En Blueprint, on met la catégorie dans le champ *Category* de la variable. C'est ce qui évite de « chercher le joueur » dans chaque tâche : une seule tâche ou un évaluateur le trouve, les autres s'y relient.

## Événements (vérifié, `StateTreeEvents.h`)

Un événement = **un Gameplay Tag** + une **charge utile** facultative (n'importe quelle struct) + une origine facultative.
- **Envoyer** : depuis un acteur, `Send State Tree Event` sur le composant State Tree ; depuis un nœud, `Send Event`.
- **Recevoir** : une transition *On Event* (tag), ou un état qui **exige** un événement pour être choisi (*Required Event To Enter*). L'événement peut être **consommé** quand il fait choisir un état (*Consume Event On Select*, présent dans tes assets).
- **Avantage** : réagir à un signal sans vérifier à chaque tick. Ton `ST_SwitchLogic` fonctionne déjà comme ça (*Transition Event*).

## Schémas (vérifié, `GameplayStateTree`)

Le schéma dit **où** tourne l'arbre et **quel contexte** il reçoit.

| Schéma | Contexte | Chez toi |
|---|---|---|
| **StateTree Component** | L'acteur (`Context Actor Class`) ; politique de tick planifié | `ST_SwitchLogic` |
| **StateTree AI Component** | L'acteur + le **contrôleur d'IA** (`AI Controller Class`) | `ST_NPC_Base`, `ST_BaseAI_Redone` |

Le composant `StateTree` démarre seul au Begin Play (*Start Logic Automatically*), ou par `Start Logic` / `Stop Logic`.

## Nœuds fournis (vérifié)

- **Conditions** : comparer un booléen, un float, un entier, un enum, un nom ; comparer une distance ; aléatoire ; objet valide, égal, de telle classe ; **Gameplay Tags** (un tag, un conteneur, une requête).
- **Considérations** : constante, entrée float, entrée enum, avec courbe de réponse.
- **Tâches** : *Delay*, *Debug Text*, *Run Parallel Tree* (lancer un autre arbre en parallèle), *Move To*, *Run Env Query* (EQS).

## Écrire ses nœuds

### En Blueprint (ce que tu fais déjà)

- **Tâche** (`StateTreeTaskBlueprintBase`) : événements *Enter State*, *Tick*, *Exit State*, *State Completed* ; terminer avec **Finish Task**. Depuis 5.x, *Enter State* et *Tick* **ne renvoient plus de statut** : l'ancienne version est dépréciée (si tu vois des avertissements, c'est ça).
- **Condition** : *Test Condition* → booléen.
- **Considération** : *Get Score* → float.
- Tous les nœuds Blueprint peuvent **envoyer un événement** et **demander une transition**.

### En C++ (plus rapide, plus propre à grande échelle)

Une tâche est une **struct**, pas un objet : pas d'objet Blueprint créé par instance, pas de machine virtuelle Blueprint à chaque tick. Les données de chaque instance sont dans une struct séparée.

```cpp
USTRUCT()
struct FMyTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AcceptRadius = 150.f;
};

USTRUCT(meta = (DisplayName = "Mon approche"))
struct FMyApproachTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FMyTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
```

Dans `Tick` : `FInstanceDataType& Data = Context.GetInstanceData(*this);`, puis renvoyer *Running*, *Succeeded* ou *Failed*. Réglages utiles de la tâche : ne pas ticker (`bShouldCallTick = false`), ou ticker seulement quand il y a des événements (`bShouldCallTickOnlyOnEvents`).

## Bonnes pratiques (pour éviter le spaghetti)

1. **L'arbre décide, les tâches font.** La forme de l'arbre exprime les choix ; le travail est dans des tâches courtes, chacune avec un rôle.
2. **Calculer une fois, relier partout** : un évaluateur (ou une tâche globale) trouve la cible et les distances ; les autres nœuds s'y relient en *Input*.
3. **Des événements plutôt que de la surveillance** : un signal extérieur (coup reçu, jauge vide, porte ouverte) arrive en événement, pas en condition testée à chaque tick.
4. **Réutiliser** : un sous-arbre « combat » partagé (*Linked*, *Linked Asset*) plutôt que des copies.
5. **Paramètres** pour que la même tâche serve à plusieurs ennemis (rayon, vitesse) au lieu de valeurs en dur.
6. **Le débogueur** : le State Tree a un débogueur intégré qui enregistre l'exécution (quels états étaient actifs, quelles transitions sont parties). `À VÉRIFIER` : où il s'ouvre exactement en 5.8 (onglet de l'éditeur de State Tree).
7. **Noms** : tes préfixes `STT_` (tâche), `ST_C_` (condition), `STEval_` (évaluateur) sont clairs. Attention, `ST_Klatt_Phoneme` (dans `MS_Voice`) est une **struct** (vérifié : `UserDefinedStruct`), pas un State Tree. Le préfixe `ST_` sert alors à deux choses ; un préfixe propre aux structs (ex. `S_`, comme `S_Stats`) éviterait la confusion.

## Où le State Tree sert dans DWAM

| Système | Usage | Remarque |
|---|---|---|
| **IA des ennemis** (existant) | Choix par utilité : poursuivre, attaquer, errer, fuir ; slots de combat (`Combat.Slot.*`) | Le cas idéal du State Tree. Brancher les **jauges** (équilibre vide → état « étourdi ») et les **phases d'un coup** |
| **Stances du joueur** ([[Plan_3C]]) | idle, interact, attack, defence, parade : transitions sur des événements d'entrée (tags `Stance.*`), tâches qui appliquent le profil (Mapping Context, mode de caméra) | À décider : State Tree (visuel, débogueur) ou machine à états en C++ (plus direct). Les deux se défendent |
| **Mécanismes** (existant : `ST_SwitchLogic`) | Leviers, interrupteurs, mécanismes à la *Zack & Wiki* : piloté par événements | Déjà dans le bon style |
| **Smart Objects** | Des objets qui **proposent** une interaction (un levier, un banc) à l'IA et au joueur ; le State Tree déroule l'interaction | Plugin *SmartObjects* (v0.1). *Gameplay Interactions*, qui relie les deux, est **expérimental** en 5.8 |
| **Feu, eau** ([[Plan_Fire]], [[Plan_Water]]) | Pas des State Trees (ce sont des simulations), mais ils peuvent **envoyer des événements** (« feu proche » → un PNJ fuit) | — |
| **Jauges** ([[Plan_Gauges]]) | Un seuil franchi envoie un événement (`Gauge.Poise` vide → étourdi) | Rejoint l'idée des seuils nommés |

## Sources (moteur 5.8)

- `Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/Public/` : `StateTreeTypes.h` (types, sélection, transitions, priorités, catégories), `StateTreeTaskBase.h`, `StateTreeConditionBase.h`, `StateTreeEvaluatorBase.h`, `StateTreeConsiderationBase.h`, `StateTreePropertyFunctionBase.h`, `StateTreeEvents.h`, `StateTreeExecutionContext.h`, `Blueprint/`, `Conditions/`, `Considerations/`, `Tasks/`.
- `Engine/Plugins/Runtime/GameplayStateTree/Source/GameplayStateTreeModule/Public/` : `Components/` (composants et schémas), `Tasks/` (*Move To*, *Run Env Query*).
