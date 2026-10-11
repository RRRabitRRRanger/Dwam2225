#include "LayoutKitBuilder.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "GraphKitTypes.h"
#include "LayoutKitTypes.h"
#include "VectorUtil.h"

using UE::Geometry::FDynamicMesh3;
using UE::Geometry::FDynamicMeshNormalOverlay;
using UE::Geometry::FDynamicMeshUVOverlay;
using UE::Geometry::FIndex3i;

namespace LayoutKit
{
	FVector2D RotateQuarter(const FVector2D& V, int32 Quarter)
	{
		switch (((Quarter % 4) + 4) % 4)
		{
		case 1:  return FVector2D(-V.Y, V.X);
		case 2:  return FVector2D(-V.X, -V.Y);
		case 3:  return FVector2D(V.Y, -V.X);
		default: return V;
		}
	}

	void ClampRadii(float Width, float Depth, float InOutRadii[4])
	{
		// Ordre : 0 avant-gauche, 1 avant-droite, 2 arrière-droite, 3 arrière-gauche
		const float MaxRadius = 0.5f * FMath::Min(Width, Depth);
		for (int32 I = 0; I < 4; ++I)
		{
			InOutRadii[I] = FMath::Clamp(InOutRadii[I], 0.f, MaxRadius);
		}

		// Deux rayons d'un même côté ne doivent pas dépasser sa longueur (sinon les arcs se croisent)
		struct FSide { int32 A; int32 B; float Length; };
		const FSide Sides[4] = { { 0, 1, Width }, { 1, 2, Depth }, { 2, 3, Width }, { 3, 0, Depth } };
		for (const FSide& Side : Sides)
		{
			const float Sum = InOutRadii[Side.A] + InOutRadii[Side.B];
			if (Sum > Side.Length && Sum > 0.f)
			{
				const float Factor = Side.Length / Sum;
				InOutRadii[Side.A] *= Factor;
				InOutRadii[Side.B] *= Factor;
			}
		}
	}

	namespace
	{
		/** Taille finale d'une boîte : chaque axe local suit l'axe de la racine sur lequel il est aligné. */
		void ResolveSizes(const FLayoutKitBox& Box, int32 Quarter, const FVector& Scale, FLayoutKitPlacedBox& Out)
		{
			const bool bAligned = (((Quarter % 2) + 2) % 2) == 0;
			const float ScaleDepth = bAligned ? Scale.X : Scale.Y;   // X local
			const float ScaleWidth = bAligned ? Scale.Y : Scale.X;   // Y local
			Out.Depth = FMath::Max(1.f, Box.Depth.Resolve(ScaleDepth));
			Out.Width = FMath::Max(1.f, Box.Width.Resolve(ScaleWidth));
			Out.Height = FMath::Max(1.f, Box.Height.Resolve(Scale.Z));
			Out.Radii[0] = Box.FrontLeftRadius;
			Out.Radii[1] = Box.FrontRightRadius;
			Out.Radii[2] = Box.BackRightRadius;
			Out.Radii[3] = Box.BackLeftRadius;
			ClampRadii(Out.Width, Out.Depth, Out.Radii);
			Out.Segments = FMath::Clamp(Box.CornerSegments, 1, 64);
		}
	}

	void SolveLayout(const FGraphKitGraph& Graph, FName Root, const FVector& Scale,
		TArray<FLayoutKitPlacedBox>& OutBoxes, TArray<FString>& OutProblems, float LOALevel)
	{
		OutBoxes.Reset();

		// Présence de chaque boîte au niveau d'abstraction demandé (en cascade depuis la racine)
		TMap<FName, float> Presence;
		LOA::ComputePresence(Graph, Root, FMath::Max(LOALevel, 0.f),
			[](const FGraphKitNode& Node) -> const FLOAElement*
			{
				const FLayoutKitBox* Box = Node.Payload.GetPtr<FLayoutKitBox>();
				return Box ? &Box->LOA : nullptr;
			},
			Presence);

		const FGraphKitNode* RootNode = Graph.FindNode(Root);
		const FLayoutKitBox* RootBox = RootNode ? RootNode->Payload.GetPtr<FLayoutKitBox>() : nullptr;
		if (!RootBox)
		{
			OutProblems.Add(FString::Printf(TEXT("La racine « %s » n'existe pas ou n'a pas de charge utile Layout Kit Box."), *Root.ToString()));
			return;
		}

		FLayoutKitPlacedBox& First = OutBoxes.AddDefaulted_GetRef();
		First.Name = Root;
		ResolveSizes(*RootBox, 0, Scale, First);

		// Parcours en largeur depuis la racine : chaque arête place un enfant contre un côté de son parent
		TSet<FName> Visited;
		Visited.Add(Root);
		TArray<FGraphKitNeighbor> Neighbors;
		for (int32 Q = 0; Q < OutBoxes.Num(); ++Q)
		{
			const FLayoutKitPlacedBox Parent = OutBoxes[Q];   // copie : le tableau peut grandir
			Graph.GetNeighbors(Parent.Name, Neighbors, EGraphKitDirection::Outgoing);

			for (const FGraphKitNeighbor& Neighbor : Neighbors)
			{
				if (Visited.Contains(Neighbor.Node))
				{
					continue;
				}
				Visited.Add(Neighbor.Node);

				const FGraphKitNode* ChildNode = Graph.FindNode(Neighbor.Node);
				const FLayoutKitBox* ChildBox = ChildNode ? ChildNode->Payload.GetPtr<FLayoutKitBox>() : nullptr;
				if (!ChildBox)
				{
					OutProblems.Add(FString::Printf(TEXT("« %s » n'a pas de charge utile Layout Kit Box : ignoré."), *Neighbor.Node.ToString()));
					continue;
				}

				const FGraphKitEdge& Edge = Graph.Edges[Neighbor.EdgeIndex];
				const FLayoutKitAttach* AttachPtr = Edge.Payload.GetPtr<FLayoutKitAttach>();
				const FLayoutKitAttach Attach = AttachPtr ? *AttachPtr : FLayoutKitAttach();

				// Le côté du parent, dans son repère local
				const float HX = 0.5f * Parent.Depth;
				const float HY = 0.5f * Parent.Width;
				FVector2D Normal, Tangent, SideCenter;
				float SideLength = 0.f;
				int32 QuarterOffset = 0;
				switch (Attach.Side)
				{
				case ELayoutKitSide::Right: Normal = { 0, 1 };  Tangent = { -1, 0 }; SideCenter = { 0, HY };  SideLength = Parent.Depth; QuarterOffset = 1; break;
				case ELayoutKitSide::Back:  Normal = { -1, 0 }; Tangent = { 0, -1 }; SideCenter = { -HX, 0 }; SideLength = Parent.Width; QuarterOffset = 2; break;
				case ELayoutKitSide::Left:  Normal = { 0, -1 }; Tangent = { 1, 0 };  SideCenter = { 0, -HY }; SideLength = Parent.Depth; QuarterOffset = 3; break;
				case ELayoutKitSide::Front:
				default:                    Normal = { 1, 0 };  Tangent = { 0, 1 };  SideCenter = { HX, 0 };  SideLength = Parent.Width; QuarterOffset = 0; break;
				}

				FLayoutKitPlacedBox Child;
				Child.Name = Neighbor.Node;
				Child.Quarter = (Parent.Quarter + QuarterOffset) % 4;
				ResolveSizes(*ChildBox, Child.Quarter, Scale, Child);

				// Niveau d'abstraction : la boîte s'efface (rapetisse depuis son attache) ou se fond dans son parent
				const float P = Presence.Contains(Neighbor.Node) ? Presence[Neighbor.Node] : 1.f;
				const bool bAbsorbed = ChildBox->LOA.Collapse == ELOACollapse::Absorb;
				if (P <= KINDA_SMALL_NUMBER && !bAbsorbed)
				{
					continue;   // effacée : ni elle ni ses enfants
				}
				// Une boîte entièrement absorbée reste : elle fait partie du volume de son parent.
				// Ses propres enfants, eux, ont une présence nulle (cascade) et s'effacent.
				float Overlap = Attach.Overlap;
				if (P < 1.f)
				{
					if (bAbsorbed)
					{
						// Elle prend la section de son parent : largeur du côté, même hauteur, coins vifs
						const float A = 1.f - P;
						Child.Width = FMath::Lerp(Child.Width, SideLength, A);
						Child.Height = FMath::Lerp(Child.Height, Parent.Height, A);
						for (float& Radius : Child.Radii)
						{
							Radius *= P;
						}
					}
					else
					{
						Child.Width *= P;
						Child.Depth *= P;
						Child.Height *= P;
						Overlap *= P;
						for (float& Radius : Child.Radii)
						{
							Radius *= P;
						}
					}
					Child.Width = FMath::Max(Child.Width, 1.f);
					Child.Depth = FMath::Max(Child.Depth, 1.f);
					Child.Height = FMath::Max(Child.Height, 1.f);
					ClampRadii(Child.Width, Child.Depth, Child.Radii);
				}

				// Alignement le long du côté (l'enfant reste dans le côté), puis avancée vers l'extérieur
				const float Slide = (FMath::Clamp(Attach.Alignment, 0.f, 1.f) - 0.5f) * FMath::Max(SideLength - Child.Width, 0.f);
				const float Out = 0.5f * Child.Depth - Overlap;
				const FVector2D Local = SideCenter + Tangent * Slide + Normal * Out;
				Child.Center = Parent.Center + RotateQuarter(Local, Parent.Quarter);
				OutBoxes.Add(Child);
			}
		}

		// Nœuds qui ne sont pas reliés à la racine
		for (const FGraphKitNode& Node : Graph.Nodes)
		{
			if (!Visited.Contains(Node.Name) && Node.Payload.GetPtr<FLayoutKitBox>())
			{
				OutProblems.Add(FString::Printf(TEXT("« %s » n'est pas relié à la racine : ignoré."), *Node.Name.ToString()));
			}
		}
	}

	void MakeExampleHouse(FGraphKitGraph& OutGraph, FName& OutRoot)
	{
		OutGraph = FGraphKitGraph();
		OutGraph.bDirected = true;
		OutRoot = TEXT("Corps");

		// Le corps : étirable, l'avant légèrement arrondi
		FLayoutKitBox Body;
		Body.Width = FLayoutKitLength(1000.f, ELayoutKitSizeRule::Stretch);
		Body.Depth = FLayoutKitLength(800.f, ELayoutKitSizeRule::Stretch);
		Body.Height = FLayoutKitLength(600.f, ELayoutKitSizeRule::Stretch);
		Body.FrontLeftRadius = 100.f;
		Body.FrontRightRadius = 100.f;
		OutGraph.AddNode(OutRoot, FGameplayTagContainer(), FInstancedStruct::Make(Body));

		// L'entrée : taille humaine, toujours là (c'est l'identité : une maison Pokémon a sa porte)
		FLayoutKitBox Entrance;
		Entrance.Width = FLayoutKitLength(300.f, ELayoutKitSizeRule::Fixed);
		Entrance.Depth = FLayoutKitLength(250.f, ELayoutKitSizeRule::Fixed);
		Entrance.Height = FLayoutKitLength(350.f, ELayoutKitSizeRule::Fixed);
		Entrance.FrontLeftRadius = 60.f;
		Entrance.FrontRightRadius = 60.f;
		OutGraph.AddNode(TEXT("Entrée"), FGameplayTagContainer(), FInstancedStruct::Make(Entrance));

		// La terrasse : un plateau bas sur le côté droit, seulement au plus détaillé
		FLayoutKitBox Terrace;
		Terrace.Width = FLayoutKitLength(500.f, ELayoutKitSizeRule::Stretch);
		Terrace.Depth = FLayoutKitLength(300.f, ELayoutKitSizeRule::Fixed);
		Terrace.Height = FLayoutKitLength(40.f, ELayoutKitSizeRule::Fixed);
		Terrace.LOA.VisibleUpTo = 0;
		Terrace.LOA.Collapse = ELOACollapse::Vanish;
		OutGraph.AddNode(TEXT("Terrasse"), FGameplayTagContainer(), FInstancedStruct::Make(Terrace));

		// La grange : collée à l'arrière, coins du fond arrondis ; au-delà du LOA 1, elle se fond dans le corps
		FLayoutKitBox Barn;
		Barn.Width = FLayoutKitLength(800.f, ELayoutKitSizeRule::Stretch);
		Barn.Depth = FLayoutKitLength(600.f, ELayoutKitSizeRule::Stretch);
		Barn.Height = FLayoutKitLength(450.f, ELayoutKitSizeRule::Stretch);
		Barn.FrontLeftRadius = 200.f;    // l'avant d'un enfant regarde vers l'extérieur : ici, le fond
		Barn.FrontRightRadius = 200.f;
		Barn.LOA.VisibleUpTo = 1;
		Barn.LOA.Collapse = ELOACollapse::Absorb;
		OutGraph.AddNode(TEXT("Grange"), FGameplayTagContainer(), FInstancedStruct::Make(Barn));

		auto Attach = [&OutGraph, &OutRoot](FName Child, ELayoutKitSide Side, float Overlap)
		{
			FLayoutKitAttach A;
			A.Side = Side;
			A.Alignment = 0.5f;
			A.Overlap = Overlap;
			OutGraph.AddEdge(OutRoot, Child, 1.f, FGameplayTagContainer(), FInstancedStruct::Make(A));
		};
		Attach(TEXT("Entrée"), ELayoutKitSide::Front, 20.f);
		Attach(TEXT("Terrasse"), ELayoutKitSide::Right, 0.f);
		Attach(TEXT("Grange"), ELayoutKitSide::Back, 20.f);
	}

	void BuildFootprint(const FLayoutKitPlacedBox& Box, TArray<FVector2D>& OutPoints, TArray<bool>& OutSharp)
	{
		OutPoints.Reset();
		OutSharp.Reset();

		const float HX = 0.5f * Box.Depth;
		const float HY = 0.5f * Box.Width;

		// Sens trigonométrique : avant-droite, arrière-droite, arrière-gauche, avant-gauche.
		// Chaque arrondi va de la face qu'on quitte à la face suivante (0°->90°, 90°->180°…).
		struct FCorner { FVector2D Corner; FVector2D Inward; float Radius; float StartDeg; };
		const FCorner Corners[4] =
		{
			{ { HX, HY },   { -1, -1 }, Box.Radii[1], 0.f },
			{ { -HX, HY },  { 1, -1 },  Box.Radii[2], 90.f },
			{ { -HX, -HY }, { 1, 1 },   Box.Radii[3], 180.f },
			{ { HX, -HY },  { -1, 1 },  Box.Radii[0], 270.f },
		};

		TArray<FVector2D> Local;
		TArray<bool> Sharp;
		for (const FCorner& C : Corners)
		{
			if (C.Radius < 0.5f)
			{
				Local.Add(C.Corner);
				Sharp.Add(true);
				continue;
			}
			const FVector2D ArcCenter = C.Corner + C.Inward * C.Radius;
			for (int32 K = 0; K <= Box.Segments; ++K)
			{
				const float Angle = FMath::DegreesToRadians(C.StartDeg + 90.f * K / Box.Segments);
				Local.Add(ArcCenter + C.Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
				Sharp.Add(false);
			}
		}

		// Points confondus (deux arrondis qui se rejoignent) : on n'en garde qu'un
		for (int32 I = 0; I < Local.Num(); ++I)
		{
			const FVector2D P = Box.Center + RotateQuarter(Local[I], Box.Quarter);
			if (OutPoints.Num() > 0 && FVector2D::Distance(P, OutPoints.Last()) < 0.01f)
			{
				OutSharp.Last() = OutSharp.Last() && Sharp[I];
				continue;
			}
			OutPoints.Add(P);
			OutSharp.Add(Sharp[I]);
		}
		if (OutPoints.Num() > 1 && FVector2D::Distance(OutPoints[0], OutPoints.Last()) < 0.01f)
		{
			OutSharp[0] = OutSharp[0] && OutSharp.Last();
			OutPoints.Pop();
			OutSharp.Pop();
		}
	}

	void AppendPrism(FDynamicMesh3& Mesh, const TArray<FVector2D>& Points, const TArray<bool>& Sharp, double Z0, double Z1)
	{
		const int32 N = Points.Num();
		if (N < 3 || !Mesh.HasAttributes())
		{
			return;
		}
		FDynamicMeshNormalOverlay* Normals = Mesh.Attributes()->PrimaryNormals();
		FDynamicMeshUVOverlay* UVs = Mesh.Attributes()->PrimaryUV();
		constexpr double UVScale = 0.01;   // 1 UV par mètre

		// Ajoute un triangle dont la face visible regarde vers Facing (convention d'Unreal : VectorUtil::Normal)
		auto AddFacing = [&](FIndex3i V, FIndex3i NE, FIndex3i UE, const FVector3d& Facing)
		{
			const FVector3d TriNormal = UE::Geometry::VectorUtil::Normal(Mesh.GetVertex(V.A), Mesh.GetVertex(V.B), Mesh.GetVertex(V.C));
			if (TriNormal.Dot(Facing) < 0.0)
			{
				Swap(V.B, V.C);
				Swap(NE.B, NE.C);
				Swap(UE.B, UE.C);
			}
			const int32 Tid = Mesh.AppendTriangle(V);
			if (Tid >= 0)
			{
				Normals->SetTriangle(Tid, NE);
				UVs->SetTriangle(Tid, UE);
			}
		};

		// Sommets : anneau bas, anneau haut, centres du sol et du toit
		TArray<int32> Bottom, Top;
		FVector2D Centroid = FVector2D::ZeroVector;
		for (int32 I = 0; I < N; ++I)
		{
			Bottom.Add(Mesh.AppendVertex(FVector3d(Points[I].X, Points[I].Y, Z0)));
			Top.Add(Mesh.AppendVertex(FVector3d(Points[I].X, Points[I].Y, Z1)));
			Centroid += Points[I] / N;
		}
		const int32 CenterBottom = Mesh.AppendVertex(FVector3d(Centroid.X, Centroid.Y, Z0));
		const int32 CenterTop = Mesh.AppendVertex(FVector3d(Centroid.X, Centroid.Y, Z1));

		// Normales des faces des murs (sens trigonométrique : l'extérieur est à droite de chaque segment)
		TArray<FVector2D> FaceNormals;
		FaceNormals.SetNum(N);
		for (int32 I = 0; I < N; ++I)
		{
			const FVector2D D = Points[(I + 1) % N] - Points[I];
			FaceNormals[I] = FVector2D(D.Y, -D.X).GetSafeNormal();
		}

		// Murs : un quad par segment ; normale lissée sur les arrondis, nette aux coins vifs
		double U = 0.0;
		for (int32 I = 0; I < N; ++I)
		{
			const int32 J = (I + 1) % N;
			const FVector2D NI = Sharp[I] ? FaceNormals[I] : (FaceNormals[(I + N - 1) % N] + FaceNormals[I]).GetSafeNormal();
			const FVector2D NJ = Sharp[J] ? FaceNormals[I] : (FaceNormals[I] + FaceNormals[J]).GetSafeNormal();
			const double Length = FVector2D::Distance(Points[I], Points[J]);

			// Un élément d'overlay appartient à un seul sommet : bas et haut ont chacun le leur
			const int32 nBI = Normals->AppendElement(FVector3f(NI.X, NI.Y, 0.f));
			const int32 nBJ = Normals->AppendElement(FVector3f(NJ.X, NJ.Y, 0.f));
			const int32 nTJ = Normals->AppendElement(FVector3f(NJ.X, NJ.Y, 0.f));
			const int32 nTI = Normals->AppendElement(FVector3f(NI.X, NI.Y, 0.f));
			const int32 uB0 = UVs->AppendElement(FVector2f(U * UVScale, Z0 * UVScale));
			const int32 uB1 = UVs->AppendElement(FVector2f((U + Length) * UVScale, Z0 * UVScale));
			const int32 uT1 = UVs->AppendElement(FVector2f((U + Length) * UVScale, Z1 * UVScale));
			const int32 uT0 = UVs->AppendElement(FVector2f(U * UVScale, Z1 * UVScale));
			U += Length;

			const FVector3d Out(FaceNormals[I].X, FaceNormals[I].Y, 0.0);
			AddFacing(FIndex3i(Bottom[I], Bottom[J], Top[J]), FIndex3i(nBI, nBJ, nTJ), FIndex3i(uB0, uB1, uT1), Out);
			AddFacing(FIndex3i(Bottom[I], Top[J], Top[I]), FIndex3i(nBI, nTJ, nTI), FIndex3i(uB0, uT1, uT0), Out);
		}

		// Sol et toit : éventail depuis le centre (l'empreinte est convexe).
		// Éléments propres à chaque sommet de chaque face (arête vive entre murs et toit).
		auto CapElements = [&](const FVector2D& P, float NormalZ, int32& OutNormal, int32& OutUV)
		{
			OutNormal = Normals->AppendElement(FVector3f(0.f, 0.f, NormalZ));
			OutUV = UVs->AppendElement(FVector2f(P.X * UVScale, P.Y * UVScale));
		};
		int32 nCB, uCB, nCT, uCT;
		CapElements(Centroid, -1.f, nCB, uCB);
		CapElements(Centroid, 1.f, nCT, uCT);
		TArray<int32> nBottom, uBottom, nTop, uTop;
		nBottom.SetNum(N); uBottom.SetNum(N); nTop.SetNum(N); uTop.SetNum(N);
		for (int32 I = 0; I < N; ++I)
		{
			CapElements(Points[I], -1.f, nBottom[I], uBottom[I]);
			CapElements(Points[I], 1.f, nTop[I], uTop[I]);
		}
		for (int32 I = 0; I < N; ++I)
		{
			const int32 J = (I + 1) % N;
			AddFacing(FIndex3i(CenterBottom, Bottom[I], Bottom[J]), FIndex3i(nCB, nBottom[I], nBottom[J]), FIndex3i(uCB, uBottom[I], uBottom[J]), FVector3d(0, 0, -1));
			AddFacing(FIndex3i(CenterTop, Top[I], Top[J]), FIndex3i(nCT, nTop[I], nTop[J]), FIndex3i(uCT, uTop[I], uTop[J]), FVector3d(0, 0, 1));
		}
	}
}
