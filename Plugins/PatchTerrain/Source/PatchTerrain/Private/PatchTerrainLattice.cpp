#include "PatchTerrainLattice.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "EngineUtils.h"
#include "PatchTerrainActor.h"

APatchTerrainLattice::APatchTerrainLattice()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetupAttachment(Root);
	Bounds->SetBoxExtent(FVector(BaseHalfSize, BaseHalfSize, BaseHeight * 0.5));
	Bounds->SetRelativeLocation(FVector(0.0, 0.0, BaseHeight * 0.5));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Bounds->SetGenerateOverlapEvents(false);
	Bounds->SetHiddenInGame(true);
	Bounds->ShapeColor = FColor(120, 200, 255);
	Bounds->SetLineThickness(4.f);
	Bounds->bIsEditorOnly = true;
}

FIntVector APatchTerrainLattice::GetClampedResolution() const
{
	return FIntVector(FMath::Clamp(Resolution.X, 2, 5), FMath::Clamp(Resolution.Y, 2, 5), FMath::Clamp(Resolution.Z, 2, 5));
}

void APatchTerrainLattice::ResetLattice()
{
	const FIntVector R = GetClampedResolution();
	ControlPoints.Reset(R.X * R.Y * R.Z);
	for (int32 K = 0; K < R.Z; ++K)
	{
		for (int32 J = 0; J < R.Y; ++J)
		{
			for (int32 I = 0; I < R.X; ++I)
			{
				// Grille régulière : avec ces points, la FFD de Bézier est l'identité
				ControlPoints.Add(FVector(
					FMath::Lerp(-BaseHalfSize, BaseHalfSize, static_cast<double>(I) / (R.X - 1)),
					FMath::Lerp(-BaseHalfSize, BaseHalfSize, static_cast<double>(J) / (R.Y - 1)),
					FMath::Lerp(0.0, BaseHeight, static_cast<double>(K) / (R.Z - 1))));
			}
		}
	}
	NotifyTerrains();
}

bool APatchTerrainLattice::AffectsTerrain(const APatchTerrain* Terrain) const
{
	return Terrain && (!TargetTerrain || TargetTerrain == Terrain);
}

FPatchTerrainLatticeDesc APatchTerrainLattice::MakeDesc(const FTransform& TerrainTransform) const
{
	FPatchTerrainLatticeDesc Desc;
	Desc.BoxToTerrain = GetActorTransform().GetRelativeTransform(TerrainTransform);
	Desc.BoxMin = FVector(-BaseHalfSize, -BaseHalfSize, 0.0);
	Desc.BoxMax = FVector(BaseHalfSize, BaseHalfSize, BaseHeight);
	Desc.Resolution = GetClampedResolution();
	Desc.ControlPoints = ControlPoints;
	Desc.Softness = Softness;
	return Desc;
}

void APatchTerrainLattice::NotifyTerrains()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<APatchTerrain> It(World); It; ++It)
	{
		It->RequestRebuild();
	}
}

void APatchTerrainLattice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FIntVector R = GetClampedResolution();
	if (ControlPoints.Num() != R.X * R.Y * R.Z)
	{
		ResetLattice();
		return;
	}
	NotifyTerrains();
}

void APatchTerrainLattice::Destroyed()
{
	Super::Destroyed();
	NotifyTerrains();
}
