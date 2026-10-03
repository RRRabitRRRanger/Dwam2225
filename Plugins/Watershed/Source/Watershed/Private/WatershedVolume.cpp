#include "WatershedVolume.h"

#include "Async/Async.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogWatershed, Log, All);

namespace WatershedVolumePrivate
{
	constexpr double CubicMetersToCm3 = 1.0e6;
	constexpr int32 Empty = -1;   // pyramide : aucune eau
	constexpr int32 Mixed = -2;   // pyramide : plusieurs lacs ou eau partielle
}

AWatershedVolume::AWatershedVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(5000.f, 5000.f, 5000.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true);
	RootComponent = Bounds;

	WaterTiles = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WaterTiles"));
	WaterTiles->SetupAttachment(Bounds);
	WaterTiles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterTiles->SetCastShadow(false);
	// Données par tuile lisibles dans le matériau (Per Instance Custom Data) : 0 = niveau (cm), 1 = indice du lac
	WaterTiles->NumCustomDataFloats = 2;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		TileMesh = PlaneFinder.Object;
		WaterTiles->SetStaticMesh(TileMesh);
	}
}

void AWatershedVolume::BeginPlay()
{
	Super::BeginPlay();

	if (bBuildOnBeginPlay)
	{
		Rebuild();
		if (InitialRain > 0.f)
		{
			AddRain(InitialRain);
		}
	}
}

// ---------------------------------------------------------------- Grille

bool AWatershedVolume::ComputeGrid(FWatershedGridDesc& OutDesc, FVector& OutOrigin) const
{
	const FBox Box = Bounds->Bounds.GetBox();
	const FVector Size = Box.GetSize();

	OutDesc.CellSize = FMath::Max(CellSize, 1.f);
	OutDesc.SizeX = FMath::Max(1, FMath::CeilToInt(Size.X / OutDesc.CellSize));
	OutDesc.SizeY = FMath::Max(1, FMath::CeilToInt(Size.Y / OutDesc.CellSize));
	OutDesc.bEdgesDrain = bEdgesDrain;
	OutOrigin = Box.Min;

	const int64 NumCells = int64(OutDesc.SizeX) * OutDesc.SizeY;
	if (NumCells > MaxCells)
	{
		UE_LOG(LogWatershed, Warning, TEXT("%s : %lld cellules (%d x %d), plus que MaxCells (%d). Réduire la boîte ou augmenter MaxCells."),
			*GetName(), NumCells, OutDesc.SizeX, OutDesc.SizeY, MaxCells);
		return false;
	}
	return true;
}

FVector AWatershedVolume::CellCenter(int32 X, int32 Y) const
{
	return GridOrigin + FVector((X + 0.5) * GridDesc.CellSize, (Y + 0.5) * GridDesc.CellSize, 0.0);
}

int32 AWatershedVolume::CellAt(const FVector& WorldLocation) const
{
	if (GridDesc.Num() == 0)
	{
		return INDEX_NONE;
	}
	const int32 X = FMath::FloorToInt((WorldLocation.X - GridOrigin.X) / GridDesc.CellSize);
	const int32 Y = FMath::FloorToInt((WorldLocation.Y - GridOrigin.Y) / GridDesc.CellSize);
	if (X < 0 || Y < 0 || X >= GridDesc.SizeX || Y >= GridDesc.SizeY)
	{
		return INDEX_NONE;
	}
	return X + Y * GridDesc.SizeX;
}

void AWatershedVolume::SampleHeights(int32 MinX, int32 MinY, int32 MaxX, int32 MaxY)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FBox Box = Bounds->Bounds.GetBox();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WatershedHeights), bTraceComplex);
	Params.AddIgnoredActor(this);
	for (AActor* Ignored : IgnoredActors)
	{
		if (Ignored)
		{
			Params.AddIgnoredActor(Ignored);
		}
	}

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const FVector Center = CellCenter(X, Y);
			const FVector Start(Center.X, Center.Y, Box.Max.Z);
			const FVector End(Center.X, Center.Y, Box.Min.Z);
			FHitResult Hit;
			// Pas de sol sous la cellule : un trou, l'eau s'en va par là
			Heights[X + Y * GridDesc.SizeX] = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params)
				? float(Hit.ImpactPoint.Z)
				: NAN;
		}
	}
}

// ---------------------------------------------------------------- Construction

void AWatershedVolume::Rebuild()
{
	FWatershedGridDesc NewDesc;
	FVector NewOrigin;
	if (!ComputeGrid(NewDesc, NewOrigin))
	{
		return;
	}

	const bool bSameGrid = NewDesc.SizeX == GridDesc.SizeX && NewDesc.SizeY == GridDesc.SizeY
		&& NewDesc.CellSize == GridDesc.CellSize && NewOrigin.Equals(GridOrigin, 0.1);
	GridDesc = NewDesc;
	GridOrigin = NewOrigin;

	if (!bSameGrid)
	{
		// Nouvelle grille : l'eau versée ne correspond plus aux mêmes cellules
		CellInputs.Init(0.0, GridDesc.Num());
	}
	Heights.SetNumUninitialized(GridDesc.Num());

	const double StartTime = FPlatformTime::Seconds();
	SampleHeights(0, 0, GridDesc.SizeX - 1, GridDesc.SizeY - 1);
	UE_LOG(LogWatershed, Log, TEXT("%s : %d x %d cellules échantillonnées en %.1f ms."),
		*GetName(), GridDesc.SizeX, GridDesc.SizeY, (FPlatformTime::Seconds() - StartTime) * 1000.0);

	StartBuild();
}

void AWatershedVolume::RebuildRegion(const FBox& WorldBox)
{
	if (GridDesc.Num() == 0 || Heights.Num() != GridDesc.Num())
	{
		Rebuild();
		return;
	}

	const int32 MinX = FMath::Clamp(FMath::FloorToInt((WorldBox.Min.X - GridOrigin.X) / GridDesc.CellSize), 0, GridDesc.SizeX - 1);
	const int32 MinY = FMath::Clamp(FMath::FloorToInt((WorldBox.Min.Y - GridOrigin.Y) / GridDesc.CellSize), 0, GridDesc.SizeY - 1);
	const int32 MaxX = FMath::Clamp(FMath::FloorToInt((WorldBox.Max.X - GridOrigin.X) / GridDesc.CellSize), 0, GridDesc.SizeX - 1);
	const int32 MaxY = FMath::Clamp(FMath::FloorToInt((WorldBox.Max.Y - GridOrigin.Y) / GridDesc.CellSize), 0, GridDesc.SizeY - 1);
	SampleHeights(MinX, MinY, MaxX, MaxY);
	StartBuild();
}

void AWatershedVolume::StartBuild()
{
	if (bBuildPending)
	{
		// Une construction tourne déjà : elle sera relancée avec les hauteurs à jour à son arrivée
		bRebuildQueued = true;
		return;
	}

	const FWatershedGridDesc Desc = GridDesc;
	TArray<float> HeightsCopy = Heights;

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		// En jeu : thread de fond, échange à l'arrivée (Tick)
		bBuildPending = true;
		PendingBuild = Async(EAsyncExecution::ThreadPool, [Desc, HeightsCopy = MoveTemp(HeightsCopy)]()
		{
			TSharedPtr<FWatershedHierarchy, ESPMode::ThreadSafe> Result = MakeShared<FWatershedHierarchy, ESPMode::ThreadSafe>();
			Result->Build(Desc, HeightsCopy);
			return Result;
		});
	}
	else
	{
		// En éditeur : immédiat
		TSharedPtr<FWatershedHierarchy, ESPMode::ThreadSafe> Result = MakeShared<FWatershedHierarchy, ESPMode::ThreadSafe>();
		const double StartTime = FPlatformTime::Seconds();
		Result->Build(Desc, HeightsCopy);
		UE_LOG(LogWatershed, Log, TEXT("%s : hiérarchie construite en %.1f ms."), *GetName(), (FPlatformTime::Seconds() - StartTime) * 1000.0);
		OnBuildFinished(Result);
	}
}

void AWatershedVolume::OnBuildFinished(TSharedPtr<FWatershedHierarchy, ESPMode::ThreadSafe> NewHierarchy)
{
	Hierarchy = NewHierarchy;

	int32 NumToOutside = 0;
	for (const FWatershedHierarchy::FNode& Node : Hierarchy->GetNodes())
	{
		NumToOutside += Node.bToOutside ? 1 : 0;
	}
	UE_LOG(LogWatershed, Log, TEXT("%s : %d creux, %d nœuds, %d bassins débordant vers l'extérieur."),
		*GetName(), Hierarchy->GetNumLeaves() - 1, Hierarchy->GetNodes().Num() - 1, NumToOutside);

	ReapplyInputs();
	UpdateTiles();
	OnHierarchyBuilt.Broadcast();
}

// ---------------------------------------------------------------- Eau

void AWatershedVolume::ReapplyInputs()
{
	if (!Hierarchy.IsValid())
	{
		return;
	}
	Hierarchy->ResetWater();
	for (int32 Cell = 0; Cell < CellInputs.Num(); ++Cell)
	{
		if (CellInputs[Cell] > 0.0)
		{
			Hierarchy->AddWater(Cell, CellInputs[Cell]);
		}
	}
	bTilesDirty = true;
}

void AWatershedVolume::AddWaterToCell(int32 Cell, double VolumeCm3)
{
	if (!CellInputs.IsValidIndex(Cell) || VolumeCm3 <= 0.0)
	{
		return;
	}
	CellInputs[Cell] += VolumeCm3;
	// Pendant une construction, l'eau est seulement mémorisée : elle sera redistribuée à l'arrivée
	if (Hierarchy.IsValid() && !bBuildPending)
	{
		Hierarchy->AddWater(Cell, VolumeCm3);
	}
	bTilesDirty = true;
}

float AWatershedVolume::AddWaterAtLocation(FVector WorldLocation, float VolumeM3)
{
	const int32 Cell = CellAt(WorldLocation);
	if (Cell == INDEX_NONE)
	{
		return VolumeM3;
	}

	const double Volume = double(VolumeM3) * WatershedVolumePrivate::CubicMetersToCm3;
	CellInputs[Cell] += Volume;
	double Lost = 0.0;
	if (Hierarchy.IsValid() && !bBuildPending)
	{
		Lost = Hierarchy->AddWater(Cell, Volume);
	}
	bTilesDirty = true;

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		UpdateTiles();
	}
	return float(Lost / WatershedVolumePrivate::CubicMetersToCm3);
}

void AWatershedVolume::AddRain(float DepthMM)
{
	if (DepthMM <= 0.f || GridDesc.Num() == 0)
	{
		return;
	}
	const double PerCell = GridDesc.CellSize * GridDesc.CellSize * (DepthMM * 0.1);   // mm -> cm
	for (int32 Cell = 0; Cell < GridDesc.Num(); ++Cell)
	{
		if (!FMath::IsNaN(Heights[Cell]))
		{
			AddWaterToCell(Cell, PerCell);
		}
	}

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		UpdateTiles();
	}
}

void AWatershedVolume::ClearWater()
{
	for (double& Input : CellInputs)
	{
		Input = 0.0;
	}
	if (Hierarchy.IsValid())
	{
		Hierarchy->ResetWater();
	}
	UpdateTiles();
}

bool AWatershedVolume::GetWaterAtLocation(FVector WorldLocation, float& OutLevel, float& OutDepth) const
{
	OutLevel = 0.f;
	OutDepth = 0.f;
	const int32 Cell = CellAt(WorldLocation);
	if (Cell == INDEX_NONE || !Hierarchy.IsValid())
	{
		return false;
	}
	const int32 Lake = Hierarchy->GetLakeOfCell(Cell);
	if (Lake == INDEX_NONE || Hierarchy->GetNodes()[Lake].Water <= 0.0)
	{
		return false;
	}
	OutLevel = float(Hierarchy->GetLevel(Lake));
	OutDepth = FMath::Max(0.f, OutLevel - Hierarchy->GetHeight(Cell));
	return OutDepth > 0.f;
}

int32 AWatershedVolume::GetNumWetLakes() const
{
	int32 Count = 0;
	if (Hierarchy.IsValid())
	{
		for (const FWatershedHierarchy::FNode& Node : Hierarchy->GetNodes())
		{
			Count += (Node.bActive && Node.Water > 0.0) ? 1 : 0;
		}
	}
	return Count;
}

// ---------------------------------------------------------------- Tick

void AWatershedVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bBuildPending && PendingBuild.IsReady())
	{
		bBuildPending = false;
		OnBuildFinished(PendingBuild.Get());
		if (bRebuildQueued)
		{
			bRebuildQueued = false;
			StartBuild();
		}
	}

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		for (const FWatershedSource& Source : Sources)
		{
			if (Source.bEnabled && Source.FlowRate > 0.f)
			{
				const int32 Cell = CellAt(GetActorTransform().TransformPosition(Source.Location));
				AddWaterToCell(Cell, double(Source.FlowRate) * DeltaSeconds * WatershedVolumePrivate::CubicMetersToCm3);
			}
		}
	}

	TimeSinceTileUpdate += DeltaSeconds;
	if (bTilesDirty && TimeSinceTileUpdate >= TileUpdateInterval && !bBuildPending)
	{
		UpdateTiles();
	}

	if (bDebugDraw)
	{
		DrawDebug();
	}
}

// ---------------------------------------------------------------- Rendu : quadtree implicite

void AWatershedVolume::UpdateTiles()
{
	using namespace WatershedVolumePrivate;

	bTilesDirty = false;
	TimeSinceTileUpdate = 0.f;
	WaterTiles->ClearInstances();
	if (TileMesh && WaterTiles->GetStaticMesh() != TileMesh)
	{
		WaterTiles->SetStaticMesh(TileMesh);
	}
	if (WaterMaterial)
	{
		WaterTiles->SetMaterial(0, WaterMaterial);
	}
	if (!Hierarchy.IsValid() || Heights.Num() != GridDesc.Num())
	{
		return;
	}

	const int32 SX = GridDesc.SizeX;
	const int32 SY = GridDesc.SizeY;
	const int32 N = GridDesc.Num();

	// Niveau 0 : lac de chaque cellule mouillée
	TArray<int32> Lake;
	Lake.Init(Empty, N);
	for (int32 Cell = 0; Cell < N; ++Cell)
	{
		const int32 L = Hierarchy->GetLakeOfCell(Cell);
		if (L != INDEX_NONE && Hierarchy->GetNodes()[L].Water > 0.0
			&& Hierarchy->GetLevel(L) - Heights[Cell] > MinDepth)
		{
			Lake[Cell] = L;
		}
	}

	// Rivage par excès : une cellule sèche voisine d'un lac, si son sol est plus haut que l'eau,
	// reçoit aussi une tuile. Le sol la cache au pixel près (test de profondeur) : rivage sans escalier.
	TArray<int32> Covered = Lake;
	for (int32 Cell = 0; Cell < N; ++Cell)
	{
		if (Lake[Cell] != Empty || FMath::IsNaN(Heights[Cell]))
		{
			continue;
		}
		const int32 X = Cell % SX;
		const int32 Y = Cell / SX;
		for (int32 DY = -1; DY <= 1 && Covered[Cell] == Empty; ++DY)
		{
			for (int32 DX = -1; DX <= 1; ++DX)
			{
				const int32 NX = X + DX;
				const int32 NY = Y + DY;
				if ((DX == 0 && DY == 0) || NX < 0 || NY < 0 || NX >= SX || NY >= SY)
				{
					continue;
				}
				const int32 L = Lake[NX + NY * SX];
				if (L != Empty && Heights[Cell] >= Hierarchy->GetLevel(L) - MinDepth)
				{
					Covered[Cell] = L;
					break;
				}
			}
		}
	}

	// Pyramide : un texel prend le lac de ses 4 enfants s'ils sont identiques, sinon « mixte »
	int32 Pow2 = 1;
	while (Pow2 < FMath::Max(SX, SY))
	{
		Pow2 *= 2;
	}
	const int32 TopLevel = FMath::Min(MaxTileLevel, int32(FMath::FloorLog2(uint32(Pow2))));

	TArray<TArray<int32>> Levels;
	Levels.SetNum(TopLevel + 1);
	Levels[0].Init(Empty, Pow2 * Pow2);
	for (int32 Y = 0; Y < SY; ++Y)
	{
		for (int32 X = 0; X < SX; ++X)
		{
			Levels[0][X + Y * Pow2] = Covered[X + Y * SX];
		}
	}
	for (int32 K = 1; K <= TopLevel; ++K)
	{
		const int32 Size = Pow2 >> K;
		const int32 ChildSize = Size * 2;
		const TArray<int32>& Child = Levels[K - 1];
		TArray<int32>& Level = Levels[K];
		Level.SetNumUninitialized(Size * Size);
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				const int32 A = Child[(2 * X) + (2 * Y) * ChildSize];
				const int32 B = Child[(2 * X + 1) + (2 * Y) * ChildSize];
				const int32 C = Child[(2 * X) + (2 * Y + 1) * ChildSize];
				const int32 D = Child[(2 * X + 1) + (2 * Y + 1) * ChildSize];
				Level[X + Y * Size] = (A == B && B == C && C == D) ? A : Mixed;
			}
		}
	}

	// Émission : un texel uniforme dont le parent est mixte = la plus grande tuile possible
	TArray<FTransform> Transforms;
	TArray<float> TileLevels;
	TArray<int32> TileLakes;
	for (int32 K = 0; K <= TopLevel; ++K)
	{
		const int32 Size = Pow2 >> K;
		const int32 TileCells = 1 << K;
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				const int32 Value = Levels[K][X + Y * Size];
				if (Value < 0)
				{
					continue;
				}
				if (K < TopLevel && Levels[K + 1][(X / 2) + (Y / 2) * (Size / 2)] != Mixed)
				{
					continue;   // le parent est uniforme : c'est lui (ou un ancêtre) qui émet
				}

				const double Level = Hierarchy->GetLevel(Value);
				const double Side = TileCells * GridDesc.CellSize;
				const FVector Center = GridOrigin + FVector((X + 0.5) * Side, (Y + 0.5) * Side, 0.0);
				Transforms.Add(FTransform(FQuat::Identity, FVector(Center.X, Center.Y, Level), FVector(Side / 100.0, Side / 100.0, 1.0)));
				TileLevels.Add(float(Level));
				TileLakes.Add(Value);
			}
		}
	}

	if (Transforms.Num() > 0)
	{
		WaterTiles->AddInstances(Transforms, false, true);
		for (int32 I = 0; I < Transforms.Num(); ++I)
		{
			WaterTiles->SetCustomDataValue(I, 0, TileLevels[I]);
			WaterTiles->SetCustomDataValue(I, 1, float(TileLakes[I]));
		}
		WaterTiles->MarkRenderStateDirty();
	}
}

// ---------------------------------------------------------------- Debug

void AWatershedVolume::DrawDebug() const
{
	UWorld* World = GetWorld();
	if (!World || !Hierarchy.IsValid())
	{
		return;
	}

	const int32 SX = GridDesc.SizeX;
	const TArray<FWatershedHierarchy::FNode>& Nodes = Hierarchy->GetNodes();

	for (int32 I = 1; I < Nodes.Num(); ++I)
	{
		const FWatershedHierarchy::FNode& Node = Nodes[I];
		if (Node.OutletCell == INDEX_NONE)
		{
			continue;
		}

		// Col : vert = débordement vers le bassin voisin, rouge = vers l'extérieur
		const FVector Cell = CellCenter(Node.OutletCell % SX, Node.OutletCell / SX);
		const FVector Outlet(Cell.X, Cell.Y, Node.OutletZ);
		DrawDebugSphere(World, Outlet, GridDesc.CellSize * 0.3f, 8, Node.bToOutside ? FColor::Red : FColor::Green, false, -1.f, 0, 2.f);

		// Lac actif avec de l'eau : niveau et volume
		if (Node.bActive && Node.Water > 0.0)
		{
			const double Level = Hierarchy->GetLevel(I);
			const FString Text = FString::Printf(TEXT("Lac %d : niveau %.2f m, %.0f m3 / %.0f m3"),
				I, Level / 100.0, Node.Water / 1.0e6, Node.Capacity < TNumericLimits<double>::Max() ? Node.Capacity / 1.0e6 : -1.0);
			DrawDebugString(World, FVector(Cell.X, Cell.Y, Level + 150.0), Text, nullptr, FColor::Cyan, 0.f, true);
		}
	}

	if (bDebugDrawHeights)
	{
		const int32 Step = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(GridDesc.Num() / 20000.0)));
		for (int32 Y = 0; Y < GridDesc.SizeY; Y += Step)
		{
			for (int32 X = 0; X < SX; X += Step)
			{
				const float H = Heights[X + Y * SX];
				if (!FMath::IsNaN(H))
				{
					const FVector C = CellCenter(X, Y);
					DrawDebugPoint(World, FVector(C.X, C.Y, H), 4.f, FColor::White, false, -1.f);
				}
			}
		}
	}
}
