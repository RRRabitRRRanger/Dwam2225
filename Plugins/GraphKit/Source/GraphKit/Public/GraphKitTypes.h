#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "GraphKitTypes.generated.h"

/** Un nœud : un nom unique dans son graphe, des tags, une charge utile libre. Aucune position. */
USTRUCT(BlueprintType)
struct GRAPHKIT_API FGraphKitNode
{
	GENERATED_BODY()

	/** Nom unique dans le graphe (« Louvre », « Entrée »…). Sert d'identifiant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	FGameplayTagContainer Tags;

	/** Données propres à l'usage (une boîte de LayoutKit, un combustible, une faction…). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	FInstancedStruct Payload;
};

/** Une arête : de From vers To, avec une distance (un poids). */
USTRUCT(BlueprintType)
struct GRAPHKIT_API FGraphKitEdge
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	FName From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	FName To;

	/** Distance, coût ou poids de l'arête (pas forcément une longueur géométrique). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph", meta = (ClampMin = "0"))
	float Distance = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	FInstancedStruct Payload;
};

UENUM(BlueprintType)
enum class EGraphKitDirection : uint8
{
	/** Les arêtes qui partent du nœud. */
	Outgoing,
	/** Les arêtes qui arrivent au nœud. */
	Incoming,
	/** Les deux. */
	Both
};

/** Un voisin d'un nœud, tel que renvoyé par GetNeighbors. */
USTRUCT(BlueprintType)
struct GRAPHKIT_API FGraphKitNeighbor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Graph")
	FName Node;

	UPROPERTY(BlueprintReadOnly, Category = "Graph")
	float Distance = 0.f;

	/** Indice de l'arête dans le graphe. */
	UPROPERTY(BlueprintReadOnly, Category = "Graph")
	int32 EdgeIndex = INDEX_NONE;

	/** Vrai si l'arête part du nœud interrogé (faux si elle y arrive). */
	UPROPERTY(BlueprintReadOnly, Category = "Graph")
	bool bOutgoing = true;
};

/**
 * Le graphe : une struct, à poser partout (variable d'acteur, Data Asset, composant).
 * Les nœuds sont identifiés par leur nom, unique dans le graphe : un graphe copié reste valide.
 */
USTRUCT(BlueprintType)
struct GRAPHKIT_API FGraphKitGraph
{
	GENERATED_BODY()

	/** Orienté : une arête va de From vers To. Non orienté : elle relie les deux dans les deux sens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	bool bDirected = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	TArray<FGraphKitNode> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph")
	TArray<FGraphKitEdge> Edges;

	// ---------------------------------------------------------------- Nœuds

	int32 FindNodeIndex(FName Name) const;
	bool HasNode(FName Name) const { return FindNodeIndex(Name) != INDEX_NONE; }
	const FGraphKitNode* FindNode(FName Name) const;
	FGraphKitNode* FindNode(FName Name);

	/** Ajoute un nœud. Échoue si le nom est vide ou déjà pris. */
	bool AddNode(FName Name, const FGameplayTagContainer& Tags = FGameplayTagContainer(), const FInstancedStruct& Payload = FInstancedStruct());

	/** Retire un nœud et toutes ses arêtes. */
	bool RemoveNode(FName Name);

	/** Renomme un nœud et met à jour ses arêtes. Échoue si le nouveau nom est vide ou déjà pris. */
	bool RenameNode(FName OldName, FName NewName);

	// ---------------------------------------------------------------- Arêtes

	/** Ajoute une arête entre deux nœuds existants (pas de boucle sur soi-même). Renvoie son indice, ou INDEX_NONE. */
	int32 AddEdge(FName From, FName To, float Distance = 1.f, const FGameplayTagContainer& Tags = FGameplayTagContainer(), const FInstancedStruct& Payload = FInstancedStruct());

	/** Retire les arêtes de From vers To (et de To vers From si le graphe n'est pas orienté). Renvoie le nombre retiré. */
	int32 RemoveEdges(FName From, FName To);

	// ---------------------------------------------------------------- Requêtes

	/** Voisins d'un nœud. Dans un graphe non orienté, toutes les arêtes comptent, quelle que soit Direction. */
	void GetNeighbors(FName Node, TArray<FGraphKitNeighbor>& OutNeighbors, EGraphKitDirection Direction = EGraphKitDirection::Outgoing) const;

	/** Nœuds dont les tags correspondent (hiérarchie comprise : « Lieu » trouve « Lieu.Repère »). */
	void GetNodesWithTag(const FGameplayTag& Tag, TArray<FName>& OutNodes) const;

	/** Indices des arêtes dont les tags correspondent. */
	void GetEdgesWithTag(const FGameplayTag& Tag, TArray<int32>& OutEdges) const;

	/** Problèmes d'écriture : noms vides ou en double, arêtes vers des nœuds absents, distances négatives. */
	void Validate(TArray<FString>& OutProblems) const;
};
