#include "WatershedHierarchy.h"

#include "Algo/BinarySearch.h"

namespace WatershedPrivate
{
	struct FFloodCell
	{
		float Key;
		int32 Cell;
	};

	struct FFloodLess
	{
		bool operator()(const FFloodCell& A, const FFloodCell& B) const { return A.Key < B.Key; }
	};

	/** Col entre deux feuilles : la hauteur la plus basse pour passer de l'une à l'autre. */
	struct FOutlet
	{
		double Z;
		int32 LabelA;
		int32 LabelB;
		int32 CellA;   // côté A
		int32 CellB;   // côté B
	};

	/** Appelle F(Voisin) pour les 4 voisins (von Neumann) dans la grille. */
	template <typename FuncType>
	FORCEINLINE void ForEachNeighbor(int32 Cell, int32 SizeX, int32 SizeY, FuncType&& F)
	{
		const int32 X = Cell % SizeX;
		const int32 Y = Cell / SizeX;
		if (X > 0)         { F(Cell - 1); }
		if (X < SizeX - 1) { F(Cell + 1); }
		if (Y > 0)         { F(Cell - SizeX); }
		if (Y < SizeY - 1) { F(Cell + SizeX); }
	}

	int32 FindRoot(TArray<int32>& UnionFind, int32 X)
	{
		int32 Root = X;
		while (UnionFind[Root] != Root)
		{
			Root = UnionFind[Root];
		}
		// Compression des chemins
		while (UnionFind[X] != Root)
		{
			const int32 Next = UnionFind[X];
			UnionFind[X] = Root;
			X = Next;
		}
		return Root;
	}
}

void FWatershedHierarchy::Build(const FWatershedGridDesc& InDesc, TArrayView<const float> InHeights)
{
	using namespace WatershedPrivate;

	Desc = InDesc;
	const int32 N = Desc.Num();
	check(InHeights.Num() == N && N > 0);

	Heights = TArray<float>(InHeights.GetData(), N);
	Labels.Init(INDEX_NONE, N);
	Nodes.Reset();
	Nodes.AddDefaulted(1);   // OutsideNode
	LostVolume = 0.0;

	// Hauteur de comparaison : un trou est plus bas que tout
	TArray<float> Z;
	Z.SetNumUninitialized(N);
	for (int32 C = 0; C < N; ++C)
	{
		Z[C] = FMath::IsNaN(Heights[C]) ? -FLT_MAX : Heights[C];
	}

	TArray<FFloodCell> Heap;
	Heap.Reserve(N);
	const FFloodLess Less;

	// 1. Extérieur : trous, et bords si la région évacue par ses bords
	for (int32 C = 0; C < N; ++C)
	{
		const int32 X = C % Desc.SizeX;
		const int32 Y = C / Desc.SizeX;
		const bool bBorder = X == 0 || Y == 0 || X == Desc.SizeX - 1 || Y == Desc.SizeY - 1;
		if (FMath::IsNaN(Heights[C]) || (Desc.bEdgesDrain && bBorder))
		{
			Labels[C] = OutsideNode;
			Heap.HeapPush(FFloodCell{ Z[C], C }, Less);
		}
	}

	// 2. Creux : cellules sans voisin strictement plus bas, regroupées par plateaux de même hauteur
	TBitArray<> IsPit(false, N);
	for (int32 C = 0; C < N; ++C)
	{
		if (Labels[C] != INDEX_NONE)
		{
			continue;
		}
		bool bPit = true;
		ForEachNeighbor(C, Desc.SizeX, Desc.SizeY, [&](int32 Nb) { bPit &= !(Z[Nb] < Z[C]); });
		IsPit[C] = bPit;
	}

	TArray<int32> Queue;
	for (int32 C = 0; C < N; ++C)
	{
		if (!IsPit[C] || Labels[C] != INDEX_NONE)
		{
			continue;
		}
		const int32 Leaf = Nodes.AddDefaulted();
		Labels[C] = Leaf;
		Queue.Reset();
		Queue.Add(C);
		for (int32 Q = 0; Q < Queue.Num(); ++Q)
		{
			const int32 Cur = Queue[Q];
			Heap.HeapPush(FFloodCell{ Z[Cur], Cur }, Less);
			ForEachNeighbor(Cur, Desc.SizeX, Desc.SizeY, [&](int32 Nb)
			{
				if (IsPit[Nb] && Labels[Nb] == INDEX_NONE && Z[Nb] == Z[Cur])
				{
					Labels[Nb] = Leaf;
					Queue.Add(Nb);
				}
			});
		}
	}
	NumLeaves = Nodes.Num();

	// 3. Remplissage par priorité : étiquettes + cols entre étiquettes voisines
	TMap<uint64, FOutlet> Outlets;
	while (Heap.Num() > 0)
	{
		FFloodCell Item;
		Heap.HeapPop(Item, Less, EAllowShrinking::No);
		const int32 C = Item.Cell;
		const int32 LabelC = Labels[C];

		ForEachNeighbor(C, Desc.SizeX, Desc.SizeY, [&](int32 Nb)
		{
			const int32 LabelN = Labels[Nb];
			if (LabelN == INDEX_NONE)
			{
				Labels[Nb] = LabelC;
				Heap.HeapPush(FFloodCell{ FMath::Max(Z[Nb], Item.Key), Nb }, Less);
			}
			else if (LabelN != LabelC)
			{
				const double OutletZ = FMath::Max(Z[C], Z[Nb]);
				const int32 A = FMath::Min(LabelC, LabelN);
				const int32 B = FMath::Max(LabelC, LabelN);
				const uint64 Key = (uint64(uint32(A)) << 32) | uint64(uint32(B));
				FOutlet* Existing = Outlets.Find(Key);
				if (!Existing || OutletZ < Existing->Z)
				{
					const bool bCIsA = LabelC == A;
					Outlets.Add(Key, FOutlet{ OutletZ, A, B, bCIsA ? C : Nb, bCIsA ? Nb : C });
				}
			}
		});
	}

	// 4. Arbre : cols du plus bas au plus haut, union-find
	TArray<FOutlet> SortedOutlets;
	Outlets.GenerateValueArray(SortedOutlets);
	SortedOutlets.Sort([](const FOutlet& A, const FOutlet& B) { return A.Z < B.Z; });

	TArray<int32> UnionFind;
	UnionFind.SetNumUninitialized(NumLeaves);
	for (int32 I = 0; I < NumLeaves; ++I)
	{
		UnionFind[I] = I;
	}

	for (const FOutlet& Outlet : SortedOutlets)
	{
		const int32 RootA = FindRoot(UnionFind, Outlet.LabelA);
		const int32 RootB = FindRoot(UnionFind, Outlet.LabelB);
		if (RootA == RootB)
		{
			continue;
		}

		if (RootA == OutsideNode || RootB == OutsideNode)
		{
			// Ce bassin déborde vers l'extérieur
			const bool bAIsOutside = RootA == OutsideNode;
			const int32 Basin = bAIsOutside ? RootB : RootA;
			FNode& Node = Nodes[Basin];
			Node.OutletZ = Outlet.Z;
			Node.OutletCell = bAIsOutside ? Outlet.CellB : Outlet.CellA;
			Node.Overflow = OutsideNode;
			Node.bToOutside = true;
			UnionFind[Basin] = OutsideNode;
		}
		else
		{
			// Fusion : un méta-bassin, les deux racines deviennent frères
			const int32 Meta = Nodes.AddDefaulted();
			UnionFind.Add(Meta);

			FNode& MetaNode = Nodes[Meta];
			MetaNode.Left = RootA;
			MetaNode.Right = RootB;

			FNode& NodeA = Nodes[RootA];
			NodeA.Parent = Meta;
			NodeA.OutletZ = Outlet.Z;
			NodeA.OutletCell = Outlet.CellA;
			NodeA.Overflow = RootB;
			NodeA.GeoLink = Outlet.LabelB;

			FNode& NodeB = Nodes[RootB];
			NodeB.Parent = Meta;
			NodeB.OutletZ = Outlet.Z;
			NodeB.OutletCell = Outlet.CellB;
			NodeB.Overflow = RootA;
			NodeB.GeoLink = Outlet.LabelA;

			UnionFind[RootA] = Meta;
			UnionFind[RootB] = Meta;
		}
	}

	// 5. Courbes volume / niveau
	ComputeLeafRanges();

	for (int32 I = 1; I < Nodes.Num(); ++I)
	{
		FNode& Node = Nodes[I];
		const bool bFinite = Node.OutletZ < TNumericLimits<double>::Max();
		Node.Capacity = bFinite ? VolumeAtLevel(I, Node.OutletZ) : TNumericLimits<double>::Max();
	}

	ResetWater();
}

void FWatershedHierarchy::ComputeLeafRanges()
{
	const int32 N = Desc.Num();

	// Cellules de chaque feuille, hauteurs triées (tri par comptage puis tri local)
	TArray<int32> Counts;
	Counts.Init(0, NumLeaves);
	for (int32 C = 0; C < N; ++C)
	{
		++Counts[Labels[C]];
	}

	LeafZStart.SetNumUninitialized(NumLeaves + 1);
	LeafZStart[0] = 0;
	for (int32 L = 0; L < NumLeaves; ++L)
	{
		LeafZStart[L + 1] = LeafZStart[L] + Counts[L];
	}

	SortedZ.SetNumUninitialized(N);
	TArray<int32> Fill = TArray<int32>(LeafZStart.GetData(), NumLeaves);
	for (int32 C = 0; C < N; ++C)
	{
		const float H = Heights[C];
		SortedZ[Fill[Labels[C]]++] = FMath::IsNaN(H) ? 0.f : H;
	}

	PrefixZ.SetNumUninitialized(N + NumLeaves);
	for (int32 L = 0; L < NumLeaves; ++L)
	{
		const int32 Start = LeafZStart[L];
		const int32 Count = LeafZStart[L + 1] - Start;
		TArrayView<float> Slice(SortedZ.GetData() + Start, Count);
		Slice.Sort();

		const int32 Base = Start + L;
		PrefixZ[Base] = 0.0;
		for (int32 K = 0; K < Count; ++K)
		{
			PrefixZ[Base + K + 1] = PrefixZ[Base + K] + Slice[K];
		}
	}

	// Parcours en profondeur (postfixe, itératif) : feuilles contiguës par sous-arbre
	LeafOrder.Reset(NumLeaves);
	TArray<TPair<int32, bool>> Stack;
	for (int32 Root = 1; Root < Nodes.Num(); ++Root)
	{
		if (Nodes[Root].Parent != INDEX_NONE)
		{
			continue;
		}

		Stack.Reset();
		Stack.Add({ Root, false });
		while (Stack.Num() > 0)
		{
			const TPair<int32, bool> Top = Stack.Pop(EAllowShrinking::No);
			const int32 I = Top.Key;
			FNode& Node = Nodes[I];

			if (I < NumLeaves)
			{
				const int32 Start = LeafZStart[I];
				const int32 Count = LeafZStart[I + 1] - Start;
				Node.LeafBegin = LeafOrder.Num();
				LeafOrder.Add(I);
				Node.LeafEnd = LeafOrder.Num();
				Node.NumCells = Count;
				Node.MinZ = Count > 0 ? SortedZ[Start] : 0.0;
				Node.MaxZ = Count > 0 ? SortedZ[Start + Count - 1] : 0.0;
			}
			else if (!Top.Value)
			{
				// Revenir sur ce nœud après ses enfants (gauche d'abord)
				Stack.Add({ I, true });
				Stack.Add({ Node.Right, false });
				Stack.Add({ Node.Left, false });
			}
			else
			{
				const FNode& L = Nodes[Node.Left];
				const FNode& R = Nodes[Node.Right];
				Node.LeafBegin = L.LeafBegin;
				Node.LeafEnd = R.LeafEnd;
				Node.NumCells = L.NumCells + R.NumCells;
				Node.MinZ = FMath::Min(L.MinZ, R.MinZ);
				Node.MaxZ = FMath::Max(L.MaxZ, R.MaxZ);
			}
		}
	}
}

double FWatershedHierarchy::VolumeAtLevel(int32 Node, double Z) const
{
	const FNode& NodeData = Nodes[Node];
	double Sum = 0.0;
	for (int32 I = NodeData.LeafBegin; I < NodeData.LeafEnd; ++I)
	{
		const int32 Leaf = LeafOrder[I];
		const int32 Start = LeafZStart[Leaf];
		const int32 Count = LeafZStart[Leaf + 1] - Start;
		if (Count == 0 || SortedZ[Start] >= Z)
		{
			continue;
		}
		// K = nombre de cellules plus basses que Z
		const int32 K = Algo::LowerBound(TArrayView<const float>(SortedZ.GetData() + Start, Count), float(Z));
		Sum += K * Z - PrefixZ[Start + Leaf + K];
	}
	return Sum * GetCellArea();
}

void FWatershedHierarchy::ResetWater()
{
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		Nodes[I].Water = 0.0;
		Nodes[I].bActive = I > 0 && I < NumLeaves;
	}
	LostVolume = 0.0;
	CachedLevel.SetNumZeroed(Nodes.Num());
	LevelValid.Init(false, Nodes.Num());
}

int32 FWatershedHierarchy::GetActiveAncestor(int32 Node) const
{
	while (Node != INDEX_NONE && !Nodes[Node].bActive)
	{
		Node = Nodes[Node].Parent;
	}
	return Node;
}

double FWatershedHierarchy::AddWater(int32 Cell, double Volume)
{
	if (Volume <= 0.0 || !Labels.IsValidIndex(Cell))
	{
		return 0.0;
	}

	const int32 Leaf = Labels[Cell];
	if (Leaf <= OutsideNode)
	{
		LostVolume += Volume;
		return Volume;
	}

	int32 Node = GetActiveAncestor(Leaf);
	double Amount = Volume;
	double Lost = 0.0;

	// Chaque tour monte d'un lac, change de frère ou fusionne : borné par la taille de l'arbre
	for (int32 Guard = 0; Amount > 0.0 && Node != INDEX_NONE && Guard < 4 * Nodes.Num(); ++Guard)
	{
		FNode& Lake = Nodes[Node];
		Lake.Water += Amount;
		Amount = 0.0;
		InvalidateLevel(Node);

		if (Lake.Water <= Lake.Capacity)
		{
			break;   // Fill : l'eau reste dans ce lac
		}

		// Plein : le surplus repart
		Amount = Lake.Water - Lake.Capacity;
		Lake.Water = Lake.Capacity;

		if (Lake.bToOutside || Lake.Overflow == INDEX_NONE)
		{
			Lost += Amount;   // Spill vers l'extérieur
			break;
		}

		FNode& Sibling = Nodes[Lake.Overflow];
		if (Sibling.bActive && Sibling.Water >= Sibling.Capacity)
		{
			// Merge : les deux frères sont pleins, ils ne forment plus qu'un lac
			const int32 Parent = Lake.Parent;
			FNode& ParentNode = Nodes[Parent];
			ParentNode.bActive = true;
			ParentNode.Water = Lake.Water + Sibling.Water;
			Lake.bActive = false;
			Sibling.bActive = false;
			InvalidateLevel(Parent);
			Node = Parent;
		}
		else
		{
			// Spill : le surplus coule dans le frère, du côté du col
			Node = GetActiveAncestor(Lake.GeoLink);
		}
	}

	LostVolume += Lost;
	return Lost;
}

int32 FWatershedHierarchy::GetLakeOfCell(int32 Cell) const
{
	if (!Labels.IsValidIndex(Cell) || Labels[Cell] <= OutsideNode)
	{
		return INDEX_NONE;
	}
	return GetActiveAncestor(Labels[Cell]);
}

double FWatershedHierarchy::GetLevel(int32 Node) const
{
	if (!Nodes.IsValidIndex(Node) || Node == OutsideNode)
	{
		return 0.0;
	}
	if (LevelValid[Node])
	{
		return CachedLevel[Node];
	}

	const FNode& NodeData = Nodes[Node];
	double Level = NodeData.MinZ;
	if (NodeData.Water > 0.0 && NodeData.NumCells > 0)
	{
		// Dichotomie sur la courbe volume / niveau (croissante)
		double Lo = NodeData.MinZ;
		double Hi = NodeData.Water <= NodeData.Capacity && NodeData.OutletZ < TNumericLimits<double>::Max()
			? NodeData.OutletZ
			: NodeData.MaxZ + NodeData.Water / (GetCellArea() * NodeData.NumCells) + 1.0;
		for (int32 Iter = 0; Iter < 60 && Hi - Lo > 0.01; ++Iter)
		{
			const double Mid = 0.5 * (Lo + Hi);
			if (VolumeAtLevel(Node, Mid) < NodeData.Water)
			{
				Lo = Mid;
			}
			else
			{
				Hi = Mid;
			}
		}
		Level = 0.5 * (Lo + Hi);
	}

	CachedLevel[Node] = Level;
	LevelValid[Node] = true;
	return Level;
}
