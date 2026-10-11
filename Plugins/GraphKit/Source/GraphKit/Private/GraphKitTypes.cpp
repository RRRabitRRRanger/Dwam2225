#include "GraphKitTypes.h"

// ---------------------------------------------------------------- Nœuds

int32 FGraphKitGraph::FindNodeIndex(FName Name) const
{
	if (Name.IsNone())
	{
		return INDEX_NONE;
	}
	return Nodes.IndexOfByPredicate([Name](const FGraphKitNode& Node) { return Node.Name == Name; });
}

const FGraphKitNode* FGraphKitGraph::FindNode(FName Name) const
{
	const int32 Index = FindNodeIndex(Name);
	return Index != INDEX_NONE ? &Nodes[Index] : nullptr;
}

FGraphKitNode* FGraphKitGraph::FindNode(FName Name)
{
	const int32 Index = FindNodeIndex(Name);
	return Index != INDEX_NONE ? &Nodes[Index] : nullptr;
}

bool FGraphKitGraph::AddNode(FName Name, const FGameplayTagContainer& Tags, const FInstancedStruct& Payload)
{
	if (Name.IsNone() || HasNode(Name))
	{
		return false;
	}
	FGraphKitNode& Node = Nodes.AddDefaulted_GetRef();
	Node.Name = Name;
	Node.Tags = Tags;
	Node.Payload = Payload;
	return true;
}

bool FGraphKitGraph::RemoveNode(FName Name)
{
	const int32 Index = FindNodeIndex(Name);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Nodes.RemoveAt(Index);
	Edges.RemoveAll([Name](const FGraphKitEdge& Edge) { return Edge.From == Name || Edge.To == Name; });
	return true;
}

bool FGraphKitGraph::RenameNode(FName OldName, FName NewName)
{
	FGraphKitNode* Node = FindNode(OldName);
	if (!Node || NewName.IsNone() || HasNode(NewName))
	{
		return false;
	}
	Node->Name = NewName;
	for (FGraphKitEdge& Edge : Edges)
	{
		if (Edge.From == OldName) { Edge.From = NewName; }
		if (Edge.To == OldName)   { Edge.To = NewName; }
	}
	return true;
}

// ---------------------------------------------------------------- Arêtes

int32 FGraphKitGraph::AddEdge(FName From, FName To, float Distance, const FGameplayTagContainer& Tags, const FInstancedStruct& Payload)
{
	if (From == To || !HasNode(From) || !HasNode(To))
	{
		return INDEX_NONE;
	}
	const int32 Index = Edges.AddDefaulted();
	FGraphKitEdge& Edge = Edges[Index];
	Edge.From = From;
	Edge.To = To;
	Edge.Distance = FMath::Max(Distance, 0.f);
	Edge.Tags = Tags;
	Edge.Payload = Payload;
	return Index;
}

int32 FGraphKitGraph::RemoveEdges(FName From, FName To)
{
	const bool bBothWays = !bDirected;
	return Edges.RemoveAll([From, To, bBothWays](const FGraphKitEdge& Edge)
	{
		return (Edge.From == From && Edge.To == To) || (bBothWays && Edge.From == To && Edge.To == From);
	});
}

// ---------------------------------------------------------------- Requêtes

void FGraphKitGraph::GetNeighbors(FName Node, TArray<FGraphKitNeighbor>& OutNeighbors, EGraphKitDirection Direction) const
{
	OutNeighbors.Reset();
	const bool bWantOut = !bDirected || Direction != EGraphKitDirection::Incoming;
	const bool bWantIn = !bDirected || Direction != EGraphKitDirection::Outgoing;

	for (int32 I = 0; I < Edges.Num(); ++I)
	{
		const FGraphKitEdge& Edge = Edges[I];
		if (bWantOut && Edge.From == Node)
		{
			FGraphKitNeighbor& N = OutNeighbors.AddDefaulted_GetRef();
			N.Node = Edge.To;
			N.Distance = Edge.Distance;
			N.EdgeIndex = I;
			N.bOutgoing = true;
		}
		else if (bWantIn && Edge.To == Node)
		{
			FGraphKitNeighbor& N = OutNeighbors.AddDefaulted_GetRef();
			N.Node = Edge.From;
			N.Distance = Edge.Distance;
			N.EdgeIndex = I;
			N.bOutgoing = false;
		}
	}
}

void FGraphKitGraph::GetNodesWithTag(const FGameplayTag& Tag, TArray<FName>& OutNodes) const
{
	OutNodes.Reset();
	for (const FGraphKitNode& Node : Nodes)
	{
		if (Node.Tags.HasTag(Tag))
		{
			OutNodes.Add(Node.Name);
		}
	}
}

void FGraphKitGraph::GetEdgesWithTag(const FGameplayTag& Tag, TArray<int32>& OutEdges) const
{
	OutEdges.Reset();
	for (int32 I = 0; I < Edges.Num(); ++I)
	{
		if (Edges[I].Tags.HasTag(Tag))
		{
			OutEdges.Add(I);
		}
	}
}

void FGraphKitGraph::Validate(TArray<FString>& OutProblems) const
{
	OutProblems.Reset();
	TSet<FName> Seen;
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		const FName Name = Nodes[I].Name;
		if (Name.IsNone())
		{
			OutProblems.Add(FString::Printf(TEXT("Nœud %d : nom vide."), I));
		}
		else if (Seen.Contains(Name))
		{
			OutProblems.Add(FString::Printf(TEXT("Nœud %d : le nom « %s » est déjà pris."), I, *Name.ToString()));
		}
		Seen.Add(Name);
	}
	for (int32 I = 0; I < Edges.Num(); ++I)
	{
		const FGraphKitEdge& Edge = Edges[I];
		if (!Seen.Contains(Edge.From))
		{
			OutProblems.Add(FString::Printf(TEXT("Arête %d : le nœud de départ « %s » n'existe pas."), I, *Edge.From.ToString()));
		}
		if (!Seen.Contains(Edge.To))
		{
			OutProblems.Add(FString::Printf(TEXT("Arête %d : le nœud d'arrivée « %s » n'existe pas."), I, *Edge.To.ToString()));
		}
		if (Edge.From == Edge.To)
		{
			OutProblems.Add(FString::Printf(TEXT("Arête %d : boucle sur « %s »."), I, *Edge.From.ToString()));
		}
		if (Edge.Distance < 0.f)
		{
			OutProblems.Add(FString::Printf(TEXT("Arête %d : distance négative."), I));
		}
	}
}
