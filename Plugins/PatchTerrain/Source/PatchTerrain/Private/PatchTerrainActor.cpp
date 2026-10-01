#include "PatchTerrainActor.h"

#include "Async/ParallelFor.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/SceneComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "PatchTerrainStamp.h"
#include "PatchTerrainLattice.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogPatchTerrain, Log, All);

using UE::Geometry::FDynamicMesh3;
using UE::Geometry::FDynamicMeshNormalOverlay;
using UE::Geometry::FDynamicMeshUVOverlay;
using UE::Geometry::FIndex3i;

namespace PatchTerrainBuild
{
	/** Positions d'échantillonnage le long d'un axe, pour les cases [First, Last) : chaque nœud est inclus, donc les bords sont partagés entre chunks. */
	static void BuildSamples(const TArray<double>& Knots, int32 First, int32 Last, double Spacing, TArray<double>& Out)
	{
		Out.Reset();
		for (int32 Cell = First; Cell < Last; ++Cell)
		{
			const double A = Knots[Cell];
			const double B = Knots[Cell + 1];
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32((B - A) / Spacing));
			for (int32 S = 0; S < Steps; ++S)
			{
				Out.Add(A + (B - A) * static_cast<double>(S) / Steps);
			}
		}
		Out.Add(Knots[Last]);
	}

	/** Réglages de la densité adaptative. */
	struct FAdaptiveSettings
	{
		/** Nombre maximal de subdivisions d'un carré de base (0 = grille régulière). */
		int32 MaxDepth = 0;
		/** Écart toléré (cm) entre la surface et l'interpolation des coins d'un carré. */
		double Tolerance = 5.0;
	};

	/**
	 * Maille le rectangle de cases [Col0, Col1) × [Row0, Row1). Thread-safe : ne lit que la surface.
	 *
	 * Chaque carré de base est subdivisé en quadtree tant que la surface s'écarte de l'interpolation de ses coins.
	 * Les sommets sont repérés sur une grille fine entière (2^MaxDepth par carré de base), ce qui permet :
	 *  - de partager les sommets entre feuilles voisines ;
	 *  - d'éviter les fissures : chaque feuille est triangulée en éventail en incluant tous les sommets
	 *    existants sur ses bords (ceux posés par des voisines plus fines) ;
	 *  - de garantir des bords de chunk identiques des deux côtés : ils sont toujours pris à la résolution fine.
	 */
	static void BuildChunkMesh(
		const FPatchTerrainSurface& Surface,
		int32 Col0, int32 Col1, int32 Row0, int32 Row1,
		double Spacing, double UVTileSize,
		const FAdaptiveSettings& Adaptive,
		FDynamicMesh3& OutMesh)
	{
		TArray<double> Xs;
		TArray<double> Ys;
		BuildSamples(Surface.GetKnotsX(), Col0, Col1, Spacing, Xs);
		BuildSamples(Surface.GetKnotsY(), Row0, Row1, Spacing, Ys);

		const int32 NBX = Xs.Num() - 1;
		const int32 NBY = Ys.Num() - 1;
		const int32 Depth = FMath::Clamp(Adaptive.MaxDepth, 0, 5);
		const int32 F = 1 << Depth;
		const int32 GX = NBX * F;
		const int32 GY = NBY * F;
		const double Tolerance = FMath::Max(Adaptive.Tolerance, 0.1);
		const double Epsilon = Spacing / F * 0.25;

		// Grille fine entière -> coordonnées du canevas
		auto ParamX = [&](int32 GI)
		{
			const int32 B = FMath::Min(GI / F, NBX - 1);
			return FMath::Lerp(Xs[B], Xs[B + 1], static_cast<double>(GI - B * F) / F);
		};
		auto ParamY = [&](int32 GJ)
		{
			const int32 B = FMath::Min(GJ / F, NBY - 1);
			return FMath::Lerp(Ys[B], Ys[B + 1], static_cast<double>(GJ - B * F) / F);
		};

		TMap<FIntPoint, FVector> PosCache;
		auto Pos = [&](int32 GI, int32 GJ) -> FVector
		{
			const FIntPoint Key(GI, GJ);
			if (const FVector* Found = PosCache.Find(Key))
			{
				return *Found;
			}
			const FVector P = Surface.Evaluate(ParamX(GI), ParamY(GJ));
			PosCache.Add(Key, P);
			return P;
		};

		// --- 1. Quadtree : on garde une feuille dès que la surface est assez proche du quadrilatère de ses coins
		struct FLeaf
		{
			int32 GI;
			int32 GJ;
			int32 Size;
		};
		TArray<FLeaf> Leaves;
		TArray<FLeaf> Stack;
		for (int32 BJ = 0; BJ < NBY; ++BJ)
		{
			for (int32 BI = 0; BI < NBX; ++BI)
			{
				Stack.Add({ BI * F, BJ * F, F });
			}
		}
		while (Stack.Num() > 0)
		{
			const FLeaf L = Stack.Pop();
			if (L.Size > 1)
			{
				const int32 S = L.Size;
				const int32 H = S / 2;
				const FVector P00 = Pos(L.GI, L.GJ);
				const FVector P10 = Pos(L.GI + S, L.GJ);
				const FVector P01 = Pos(L.GI, L.GJ + S);
				const FVector P11 = Pos(L.GI + S, L.GJ + S);

				double Err = FVector::Dist(Pos(L.GI + H, L.GJ), (P00 + P10) * 0.5);
				Err = FMath::Max(Err, FVector::Dist(Pos(L.GI, L.GJ + H), (P00 + P01) * 0.5));
				Err = FMath::Max(Err, FVector::Dist(Pos(L.GI + S, L.GJ + H), (P10 + P11) * 0.5));
				Err = FMath::Max(Err, FVector::Dist(Pos(L.GI + H, L.GJ + S), (P01 + P11) * 0.5));
				Err = FMath::Max(Err, FVector::Dist(Pos(L.GI + H, L.GJ + H), (P00 + P10 + P01 + P11) * 0.25));

				if (Err > Tolerance)
				{
					Stack.Add({ L.GI, L.GJ, H });
					Stack.Add({ L.GI + H, L.GJ, H });
					Stack.Add({ L.GI, L.GJ + H, H });
					Stack.Add({ L.GI + H, L.GJ + H, H });
					continue;
				}
			}
			Leaves.Add(L);
		}

		// --- 2. Sommets présents : coins des feuilles + bords du chunk à la résolution fine
		TSet<FIntPoint> Used;
		for (const FLeaf& L : Leaves)
		{
			Used.Add(FIntPoint(L.GI, L.GJ));
			Used.Add(FIntPoint(L.GI + L.Size, L.GJ));
			Used.Add(FIntPoint(L.GI, L.GJ + L.Size));
			Used.Add(FIntPoint(L.GI + L.Size, L.GJ + L.Size));
		}
		if (Depth > 0)
		{
			for (int32 GI = 0; GI <= GX; ++GI)
			{
				Used.Add(FIntPoint(GI, 0));
				Used.Add(FIntPoint(GI, GY));
			}
			for (int32 GJ = 0; GJ <= GY; ++GJ)
			{
				Used.Add(FIntPoint(0, GJ));
				Used.Add(FIntPoint(GX, GJ));
			}
		}

		// --- 3. Maillage
		OutMesh = FDynamicMesh3();
		OutMesh.EnableAttributes();
		FDynamicMeshNormalOverlay* Normals = OutMesh.Attributes()->PrimaryNormals();
		FDynamicMeshUVOverlay* UVs = OutMesh.Attributes()->PrimaryUV();

		// Un élément de normale et d'UV par sommet, créés ensemble : les identifiants coïncident
		TMap<FIntPoint, int32> VertexIds;
		auto GetVertex = [&](const FIntPoint& Key) -> int32
		{
			if (const int32* Found = VertexIds.Find(Key))
			{
				return *Found;
			}
			const double X = ParamX(Key.X);
			const double Y = ParamY(Key.Y);
			FVector Position;
			FVector Normal;
			Surface.EvaluateWithNormal(X, Y, Epsilon, Position, Normal);

			const int32 Vid = OutMesh.AppendVertex(Position);
			Normals->AppendElement(FVector3f(Normal));
			// UV dans l'espace du canevas : stables même quand on déplace les points en XY
			UVs->AppendElement(FVector2f(static_cast<float>(X / UVTileSize), static_cast<float>(Y / UVTileSize)));
			VertexIds.Add(Key, Vid);
			return Vid;
		};

		auto AddTriangle = [&](int32 A, int32 B, int32 C)
		{
			const FIndex3i Tri(A, B, C);
			const int32 Tid = OutMesh.AppendTriangle(Tri);
			if (Tid >= 0)
			{
				Normals->SetTriangle(Tid, Tri);
				UVs->SetTriangle(Tid, Tri);
			}
		};

		// Enroulement : la normale du triangle (convention main gauche d'Unreal) pointe vers +Z
		TArray<int32> Ring;
		for (const FLeaf& L : Leaves)
		{
			const int32 X0 = L.GI;
			const int32 Y0 = L.GJ;
			const int32 X1 = L.GI + L.Size;
			const int32 Y1 = L.GJ + L.Size;

			// Contour de la feuille (X croissant puis Y croissant, etc.), avec tous les sommets présents sur ses bords
			Ring.Reset();
			for (int32 X = X0; X < X1; ++X) { if (Used.Contains(FIntPoint(X, Y0))) { Ring.Add(GetVertex(FIntPoint(X, Y0))); } }
			for (int32 Y = Y0; Y < Y1; ++Y) { if (Used.Contains(FIntPoint(X1, Y))) { Ring.Add(GetVertex(FIntPoint(X1, Y))); } }
			for (int32 X = X1; X > X0; --X) { if (Used.Contains(FIntPoint(X, Y1))) { Ring.Add(GetVertex(FIntPoint(X, Y1))); } }
			for (int32 Y = Y1; Y > Y0; --Y) { if (Used.Contains(FIntPoint(X0, Y))) { Ring.Add(GetVertex(FIntPoint(X0, Y))); } }

			if (Ring.Num() == 4)
			{
				// Carré simple : diagonale la plus courte en 3D (suit les falaises au lieu de les traverser)
				const int32 V00 = Ring[0];
				const int32 V10 = Ring[1];
				const int32 V11 = Ring[2];
				const int32 V01 = Ring[3];
				const bool bMainDiagonal =
					FVector::DistSquared(OutMesh.GetVertex(V00), OutMesh.GetVertex(V11)) <=
					FVector::DistSquared(OutMesh.GetVertex(V10), OutMesh.GetVertex(V01));
				if (bMainDiagonal)
				{
					AddTriangle(V00, V11, V10);
					AddTriangle(V00, V01, V11);
				}
				else
				{
					AddTriangle(V00, V01, V10);
					AddTriangle(V10, V01, V11);
				}
			}
			else
			{
				// Une voisine plus fine a posé des sommets sur un bord : éventail depuis le centre (pas de jonction en T)
				const int32 Center = GetVertex(FIntPoint(X0 + L.Size / 2, Y0 + L.Size / 2));
				for (int32 K = 0; K < Ring.Num(); ++K)
				{
					AddTriangle(Center, Ring[(K + 1) % Ring.Num()], Ring[K]);
				}
			}
		}
	}
}

APatchTerrain::APatchTerrain()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	// Canevas par défaut : 8 × 8 cases de 25 m (200 m de côté), à plat
	ColumnWidths.Init(2500.f, 8);
	RowHeights.Init(2500.f, 8);
}

void APatchTerrain::BuildKnots(TArray<double>& OutKnotsX, TArray<double>& OutKnotsY) const
{
	auto Accumulate = [](const TArray<float>& Sizes, TArray<double>& Out)
	{
		Out.Reset(Sizes.Num() + 1);
		double Sum = 0.0;
		Out.Add(Sum);
		for (const float Size : Sizes)
		{
			Sum += FMath::Max(1.0, static_cast<double>(Size));
			Out.Add(Sum);
		}
	};
	Accumulate(ColumnWidths, OutKnotsX);
	Accumulate(RowHeights, OutKnotsY);
}

void APatchTerrain::EnsureControlPoints()
{
	const int32 Expected = (ColumnWidths.Num() + 1) * (RowHeights.Num() + 1);
	if (ColumnWidths.Num() > 0 && RowHeights.Num() > 0 && ControlPoints.Num() != Expected)
	{
		if (ControlPoints.Num() > 0)
		{
			UE_LOG(LogPatchTerrain, Warning, TEXT("%s : la grille a changé de taille, points de contrôle réinitialisés à plat."), *GetName());
		}
		ResetControlPoints();
	}
}

void APatchTerrain::ResetCanvas()
{
	ResetControlPoints();
	if (bAutoRebuildInEditor)
	{
		Rebuild();
	}
}

void APatchTerrain::ResetControlPoints()
{
	TArray<double> KX;
	TArray<double> KY;
	BuildKnots(KX, KY);

	ControlPoints.Reset(KX.Num() * KY.Num());
	for (const double Y : KY)
	{
		for (const double X : KX)
		{
			ControlPoints.Add(FVector(X, Y, 0.0));
		}
	}
}

void APatchTerrain::RealignToGrid()
{
	TArray<double> KX;
	TArray<double> KY;
	BuildKnots(KX, KY);

	if (ControlPoints.Num() != KX.Num() * KY.Num())
	{
		ResetCanvas();
		return;
	}

	for (int32 J = 0; J < KY.Num(); ++J)
	{
		for (int32 I = 0; I < KX.Num(); ++I)
		{
			FVector& P = ControlPoints[J * KX.Num() + I];
			P.X = KX[I];
			P.Y = KY[J];
		}
	}

	if (bAutoRebuildInEditor)
	{
		Rebuild();
	}
}

int32 APatchTerrain::GetEffectiveSeed() const
{
	return RuntimeSeed.IsSet() ? RuntimeSeed.GetValue() : Seed;
}

UDynamicMeshComponent* APatchTerrain::CreateChunkComponent()
{
	// Transitoire : jamais sauvegardé ni dupliqué, le terrain est toujours recalculé depuis ses données
	UDynamicMeshComponent* Comp = NewObject<UDynamicMeshComponent>(this, NAME_None, RF_Transient);
	Comp->SetMobility(EComponentMobility::Static);
	Comp->SetupAttachment(RootComponent);
	Comp->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
	Comp->RegisterComponent();
	return Comp;
}

void APatchTerrain::DestroyChunkComponents()
{
	for (UDynamicMeshComponent* Comp : ChunkComponents)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
	}
	ChunkComponents.Reset();
}

void APatchTerrain::Rebuild()
{
	EnsureControlPoints();

	TArray<double> KX;
	TArray<double> KY;
	BuildKnots(KX, KY);

	TSharedPtr<FPatchTerrainSurface> Surface = MakeShared<FPatchTerrainSurface>();
	TArray<FString> ModifierStatus;
	const bool bSurfaceOk = Surface->Setup(MoveTemp(KX), MoveTemp(KY), ControlPoints, Modifiers, GetEffectiveSeed(), &ModifierStatus);

	// Retour visible dans le panneau Détails pour chaque expression
	for (int32 Index = 0; Index < Modifiers.Num(); ++Index)
	{
		FPatchTerrainModifier& Mod = Modifiers[Index];
		if (Mod.Type != EPatchTerrainModifierType::Expression)
		{
			Mod.ExpressionStatus.Reset();
			continue;
		}
		Mod.ExpressionStatus = !Mod.bEnabled ? FString(TEXT("Désactivé")) : (ModifierStatus.IsValidIndex(Index) ? ModifierStatus[Index] : FString());
		if (Mod.ExpressionStatus.StartsWith(TEXT("Erreur")))
		{
			UE_LOG(LogPatchTerrain, Warning, TEXT("%s : modificateur %d ignoré. %s"), *GetName(), Index, *Mod.ExpressionStatus);
		}
	}

	if (!bSurfaceOk)
	{
		UE_LOG(LogPatchTerrain, Warning, TEXT("%s : canevas invalide, rien à construire."), *GetName());
		CachedSurface.Reset();
		DestroyChunkComponents();
		return;
	}
	// Tampons du niveau qui visent ce terrain, par priorité croissante
	TArray<APatchTerrainStamp*> StampActors;
	if (UWorld* StampWorld = GetWorld())
	{
		for (TActorIterator<APatchTerrainStamp> It(StampWorld); It; ++It)
		{
			if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->AffectsTerrain(this))
			{
				StampActors.Add(*It);
			}
		}
	}
	StampActors.StableSort([](const APatchTerrainStamp& A, const APatchTerrainStamp& B) { return A.Priority < B.Priority; });

	TArray<FPatchTerrainStampDesc> StampDescs;
	StampDescs.Reserve(StampActors.Num());
	for (const APatchTerrainStamp* Stamp : StampActors)
	{
		StampDescs.Add(Stamp->MakeDesc(GetActorTransform()));
	}

	// Lattices du niveau qui visent ce terrain
	TArray<FPatchTerrainLatticeDesc> LatticeDescs;
	if (UWorld* LatticeWorld = GetWorld())
	{
		for (TActorIterator<APatchTerrainLattice> It(LatticeWorld); It; ++It)
		{
			if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->AffectsTerrain(this))
			{
				LatticeDescs.Add(It->MakeDesc(GetActorTransform()));
			}
		}
	}
	Surface->SetLattices(LatticeDescs);

	TArray<FString> StampStatus;
	Surface->SetStamps(StampDescs, GetEffectiveSeed(), &StampStatus);
	for (int32 Index = 0; Index < StampActors.Num(); ++Index)
	{
		APatchTerrainStamp* Stamp = StampActors[Index];
		Stamp->ExpressionStatus = Stamp->Shape == EPatchTerrainStampShape::Expression ? StampStatus[Index] : FString();
		if (Stamp->ExpressionStatus.StartsWith(TEXT("Erreur")))
		{
			UE_LOG(LogPatchTerrain, Warning, TEXT("%s : tampon %s ignoré. %s"), *GetName(), *Stamp->GetName(), *Stamp->ExpressionStatus);
		}
	}

	CachedSurface = Surface;

	const int32 NumCols = GetNumColumns();
	const int32 NumRows = GetNumRows();
	const int32 CellsX = FMath::Max(1, ChunkCells.X);
	const int32 CellsY = FMath::Max(1, ChunkCells.Y);
	const int32 NumChunksX = FMath::DivideAndRoundUp(NumCols, CellsX);
	const int32 NumChunksY = FMath::DivideAndRoundUp(NumRows, CellsY);
	const int32 NumChunks = NumChunksX * NumChunksY;

	const double Spacing = FMath::Max(10.0, static_cast<double>(SampleSpacing));
	const double UVSize = FMath::Max(1.0, static_cast<double>(UVTileSize));

	PatchTerrainBuild::FAdaptiveSettings Adaptive;
	Adaptive.MaxDepth = AdaptiveMaxDepth;
	Adaptive.Tolerance = AdaptiveTolerance;

	// Maillage en parallèle : la surface est en lecture seule
	TArray<FDynamicMesh3> Meshes;
	Meshes.SetNum(NumChunks);
	ParallelFor(NumChunks, [&](int32 Index)
	{
		const int32 CX = Index % NumChunksX;
		const int32 CY = Index / NumChunksX;
		const int32 Col0 = CX * CellsX;
		const int32 Row0 = CY * CellsY;
		const int32 Col1 = FMath::Min(Col0 + CellsX, NumCols);
		const int32 Row1 = FMath::Min(Row0 + CellsY, NumRows);
		PatchTerrainBuild::BuildChunkMesh(*Surface, Col0, Col1, Row0, Row1, Spacing, UVSize, Adaptive, Meshes[Index]);
	});

	// Ajuste le nombre de composants
	ChunkComponents.RemoveAll([](const TObjectPtr<UDynamicMeshComponent>& Comp) { return !IsValid(Comp); });
	while (ChunkComponents.Num() > NumChunks)
	{
		ChunkComponents.Pop()->DestroyComponent();
	}
	while (ChunkComponents.Num() < NumChunks)
	{
		ChunkComponents.Add(CreateChunkComponent());
	}

	const UWorld* World = GetWorld();
	const bool bGameWorld = World && World->IsGameWorld();

	for (int32 Index = 0; Index < NumChunks; ++Index)
	{
		UDynamicMeshComponent* Comp = ChunkComponents[Index];

		Comp->SetMesh(MoveTemp(Meshes[Index]));
		Comp->ConfigureMaterialSet({ Material.Get() });

		if (bGenerateCollision)
		{
			// En jeu, la cuisson de la collision se fait en tâche de fond
			Comp->bUseAsyncCooking = bGameWorld;
			Comp->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Comp->SetComplexAsSimpleCollisionEnabled(true, true);
		}
		else
		{
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetComplexAsSimpleCollisionEnabled(false, true);
		}
	}
}

void APatchTerrain::RequestRebuild()
{
#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (GEditor && World && !World->IsGameWorld())
	{
		if (!bAutoRebuildInEditor || bRebuildPending)
		{
			return;
		}
		bRebuildPending = true;
		GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			bRebuildPending = false;
			Rebuild();
		}));
		return;
	}
#endif
	Rebuild();
}

void APatchTerrain::RegenerateWithSeed(int32 NewSeed)
{
	RuntimeSeed = NewSeed;
	Rebuild();
}

void APatchTerrain::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const UWorld* World = GetWorld();
	const bool bGameWorld = World && World->IsGameWorld();
	if (bAutoRebuildInEditor || bGameWorld)
	{
		Rebuild();
	}
}

void APatchTerrain::BeginPlay()
{
	Super::BeginPlay();

	if (bRandomSeedAtRuntime)
	{
		RuntimeSeed = FMath::Rand();
		UE_LOG(LogPatchTerrain, Log, TEXT("%s : seed aléatoire %d"), *GetName(), RuntimeSeed.GetValue());
		Rebuild();
	}
	else if (ChunkComponents.Num() == 0)
	{
		// Acteur placé dans un niveau chargé en jeu : les composants transitoires n'existent pas encore
		Rebuild();
	}
}

bool APatchTerrain::GetSurfacePoint(FVector2D CanvasPosition, FVector& OutWorldPosition, FVector& OutWorldNormal) const
{
	if (!CachedSurface.IsValid() || !CachedSurface->IsValid())
	{
		return false;
	}

	FVector LocalPos;
	FVector LocalNormal;
	CachedSurface->EvaluateWithNormal(CanvasPosition.X, CanvasPosition.Y, FMath::Max(10.0, static_cast<double>(SampleSpacing) * 0.25), LocalPos, LocalNormal);

	const FTransform& Xf = GetActorTransform();
	OutWorldPosition = Xf.TransformPosition(LocalPos);
	OutWorldNormal = Xf.TransformVectorNoScale(LocalNormal).GetSafeNormal();
	return true;
}

FVector2D APatchTerrain::GetCanvasSize() const
{
	double SX = 0.0;
	double SY = 0.0;
	for (const float W : ColumnWidths)
	{
		SX += FMath::Max(1.0, static_cast<double>(W));
	}
	for (const float H : RowHeights)
	{
		SY += FMath::Max(1.0, static_cast<double>(H));
	}
	return FVector2D(SX, SY);
}
