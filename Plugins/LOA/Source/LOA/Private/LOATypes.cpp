#include "LOATypes.h"

#include "GraphKitTypes.h"

namespace LOA
{
	void ComputePresence(const FGraphKitGraph& Graph, FName Root, float Level,
		TFunctionRef<const FLOAElement*(const FGraphKitNode&)> GetElement, TMap<FName, float>& OutPresence)
	{
		OutPresence.Reset();
		if (!Graph.HasNode(Root))
		{
			return;
		}

		// Parcours en largeur depuis la racine : présence = min(propre, parent)
		OutPresence.Add(Root, 1.f);
		TArray<FName> Queue = { Root };
		TArray<FGraphKitNeighbor> Neighbors;
		for (int32 Q = 0; Q < Queue.Num(); ++Q)
		{
			const FName Parent = Queue[Q];
			const float ParentPresence = OutPresence.FindChecked(Parent);
			Graph.GetNeighbors(Parent, Neighbors, EGraphKitDirection::Outgoing);
			for (const FGraphKitNeighbor& Neighbor : Neighbors)
			{
				if (OutPresence.Contains(Neighbor.Node))
				{
					continue;
				}
				const FGraphKitNode* Node = Graph.FindNode(Neighbor.Node);
				const FLOAElement* Element = Node ? GetElement(*Node) : nullptr;
				const float Own = Element ? Element->Presence(Level) : 1.f;
				OutPresence.Add(Neighbor.Node, FMath::Min(Own, ParentPresence));
				Queue.Add(Neighbor.Node);
			}
		}
	}
}
