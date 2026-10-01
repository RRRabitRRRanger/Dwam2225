#include "PatchTerrainStamp.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "EngineUtils.h"
#include "PatchTerrainActor.h"

APatchTerrainStamp::APatchTerrainStamp()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Boîte filaire : emprise au sol et hauteur (posée sur la base du tampon)
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetupAttachment(Root);
	Bounds->SetBoxExtent(FVector(BaseHalfSize, BaseHalfSize, BaseHeight * 0.5));
	Bounds->SetRelativeLocation(FVector(0.0, 0.0, BaseHeight * 0.5));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Bounds->SetGenerateOverlapEvents(false);
	Bounds->SetHiddenInGame(true);
	Bounds->ShapeColor = FColor(255, 170, 40);
	Bounds->SetLineThickness(4.f);
	Bounds->bIsEditorOnly = true;
}

bool APatchTerrainStamp::AffectsTerrain(const APatchTerrain* Terrain) const
{
	return Terrain && (!TargetTerrain || TargetTerrain == Terrain);
}

FPatchTerrainStampDesc APatchTerrainStamp::MakeDesc(const FTransform& TerrainTransform) const
{
	const FTransform Rel = GetActorTransform().GetRelativeTransform(TerrainTransform);
	const FVector Scale = Rel.GetScale3D();

	FPatchTerrainStampDesc Desc;
	Desc.Shape = Shape;
	Desc.Blend = Blend;
	Desc.Center = Rel.GetLocation();
	Desc.Yaw = Rel.Rotator().Yaw;
	Desc.HalfSize = FVector2D(BaseHalfSize * FMath::Abs(Scale.X), BaseHalfSize * FMath::Abs(Scale.Y));
	Desc.Height = BaseHeight * Scale.Z;
	Desc.Squareness = Squareness;
	Desc.EdgeSoftness = EdgeSoftness;
	Desc.Frequency = Frequency;
	Desc.Shaping = Shaping;
	Desc.Expression = Expression;
	Desc.bAutoFitEnds = bAutoFitEnds;
	Desc.StepCount = StepCount;
	Desc.StepSoftness = StepSoftness;
	Desc.NoiseType = NoiseType;
	Desc.NoiseAmount = NoiseAmount;
	Desc.NoiseScale = NoiseScale;
	Desc.NoiseOctaves = NoiseOctaves;
	Desc.Seed = Seed;
	return Desc;
}

void APatchTerrainStamp::NotifyTerrains()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Tous les terrains du niveau : celui que le tampon quitte doit aussi se mettre à jour
	for (TActorIterator<APatchTerrain> It(World); It; ++It)
	{
		It->RequestRebuild();
	}
}

void APatchTerrainStamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	NotifyTerrains();
}

void APatchTerrainStamp::Destroyed()
{
	Super::Destroyed();
	NotifyTerrains();
}

#if WITH_EDITOR
void APatchTerrainStamp::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = PropertyChangedEvent.GetMemberPropertyName();
	if (bAutoLabel && (Name == GET_MEMBER_NAME_CHECKED(APatchTerrainStamp, Shape) || Name == GET_MEMBER_NAME_CHECKED(APatchTerrainStamp, bAutoLabel)))
	{
		const FText ShapeName = StaticEnum<EPatchTerrainStampShape>()->GetDisplayNameTextByValue(static_cast<int64>(Shape));
		SetActorLabel(FString::Printf(TEXT("Stamp_%s"), *ShapeName.ToString()));
	}
}
#endif
