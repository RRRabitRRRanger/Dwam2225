#include "GraphKitLibrary.h"

bool UGraphKitLibrary::AddNode(FGraphKitGraph& Graph, FName Name, FGameplayTagContainer Tags, FInstancedStruct Payload)
{
	return Graph.AddNode(Name, Tags, Payload);
}

bool UGraphKitLibrary::RemoveNode(FGraphKitGraph& Graph, FName Name)
{
	return Graph.RemoveNode(Name);
}

bool UGraphKitLibrary::RenameNode(FGraphKitGraph& Graph, FName OldName, FName NewName)
{
	return Graph.RenameNode(OldName, NewName);
}

int32 UGraphKitLibrary::AddEdge(FGraphKitGraph& Graph, FName From, FName To, float Distance, FGameplayTagContainer Tags, FInstancedStruct Payload)
{
	return Graph.AddEdge(From, To, Distance, Tags, Payload);
}

int32 UGraphKitLibrary::RemoveEdges(FGraphKitGraph& Graph, FName From, FName To)
{
	return Graph.RemoveEdges(From, To);
}

bool UGraphKitLibrary::FindNode(const FGraphKitGraph& Graph, FName Name, FGraphKitNode& OutNode)
{
	if (const FGraphKitNode* Node = Graph.FindNode(Name))
	{
		OutNode = *Node;
		return true;
	}
	return false;
}

TArray<FGraphKitNeighbor> UGraphKitLibrary::GetNeighbors(const FGraphKitGraph& Graph, FName Node, EGraphKitDirection Direction)
{
	TArray<FGraphKitNeighbor> Result;
	Graph.GetNeighbors(Node, Result, Direction);
	return Result;
}

TArray<FName> UGraphKitLibrary::GetNodesWithTag(const FGraphKitGraph& Graph, FGameplayTag Tag)
{
	TArray<FName> Result;
	Graph.GetNodesWithTag(Tag, Result);
	return Result;
}

TArray<FString> UGraphKitLibrary::Validate(const FGraphKitGraph& Graph)
{
	TArray<FString> Result;
	Graph.Validate(Result);
	return Result;
}
