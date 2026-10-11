#pragma once

#include "CoreMinimal.h"
#include "GraphKitTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GraphKitLibrary.generated.h"

/** Opérations Blueprint sur un graphe (la struct est passée par référence). */
UCLASS()
class GRAPHKIT_API UGraphKitLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GraphKit")
	static bool AddNode(UPARAM(ref) FGraphKitGraph& Graph, FName Name, FGameplayTagContainer Tags, FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable, Category = "GraphKit")
	static bool RemoveNode(UPARAM(ref) FGraphKitGraph& Graph, FName Name);

	UFUNCTION(BlueprintCallable, Category = "GraphKit")
	static bool RenameNode(UPARAM(ref) FGraphKitGraph& Graph, FName OldName, FName NewName);

	/** Renvoie l'indice de la nouvelle arête, ou -1. */
	UFUNCTION(BlueprintCallable, Category = "GraphKit")
	static int32 AddEdge(UPARAM(ref) FGraphKitGraph& Graph, FName From, FName To, float Distance, FGameplayTagContainer Tags, FInstancedStruct Payload);

	UFUNCTION(BlueprintCallable, Category = "GraphKit")
	static int32 RemoveEdges(UPARAM(ref) FGraphKitGraph& Graph, FName From, FName To);

	UFUNCTION(BlueprintPure, Category = "GraphKit")
	static bool FindNode(const FGraphKitGraph& Graph, FName Name, FGraphKitNode& OutNode);

	UFUNCTION(BlueprintPure, Category = "GraphKit")
	static TArray<FGraphKitNeighbor> GetNeighbors(const FGraphKitGraph& Graph, FName Node, EGraphKitDirection Direction = EGraphKitDirection::Outgoing);

	UFUNCTION(BlueprintPure, Category = "GraphKit")
	static TArray<FName> GetNodesWithTag(const FGraphKitGraph& Graph, FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "GraphKit")
	static TArray<FString> Validate(const FGraphKitGraph& Graph);
};
