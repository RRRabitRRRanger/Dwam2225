#include "SnakeToy.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace SnakeToyLocal
{
	/** Le mesh BasicShapes/Sphere a un rayon de 50 cm. */
	constexpr float SphereMeshRadius = 50.f;

	/** Pas minimal entre deux points enregistrés du chemin (cm). */
	constexpr float PathStep = 2.f;

	/** Direction tangente à Up, la plus proche possible de Preferred. */
	static FVector MakeTangent(const FVector& Preferred, const FVector& Up)
	{
		FVector T = FVector::VectorPlaneProject(Preferred, Up);
		if (T.Normalize())
		{
			return T;
		}
		T = FVector::VectorPlaneProject(FVector::ForwardVector, Up);
		if (T.Normalize())
		{
			return T;
		}
		return FVector::VectorPlaneProject(FVector::RightVector, Up).GetSafeNormal();
	}
}

ASnakeToy::ASnakeToy()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	SphereMesh = SphereFinder.Object;
}

float ASnakeToy::GetSegmentRadius(int32 Index) const
{
	const float Alpha = NumSegments > 1 ? static_cast<float>(Index) / (NumSegments - 1) : 0.f;
	return FMath::Lerp(HeadRadius, HeadRadius * TailRadiusRatio, Alpha);
}

void ASnakeToy::EnsureSegments()
{
	Segments.RemoveAll([](const TObjectPtr<UStaticMeshComponent>& Comp) { return !IsValid(Comp); });

	while (Segments.Num() > NumSegments)
	{
		Segments.Pop()->DestroyComponent();
	}
	while (Segments.Num() < NumSegments)
	{
		// Transitoire : le snake est toujours reconstruit depuis ses réglages
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(SphereMesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetupAttachment(RootComponent);
		Comp->RegisterComponent();
		Segments.Add(Comp);
	}

	for (UStaticMeshComponent* Comp : Segments)
	{
		if (Material)
		{
			Comp->SetMaterial(0, Material);
		}
	}
}

bool ASnakeToy::Trace(const FVector& Start, const FVector& End, FHitResult& OutHit) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Traces complexes : on suit la géométrie visuelle (les triangles), pas les boîtes de collision simples
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SnakeToyTrace), /*bTraceComplex*/ true, this);
	const bool bHit = World->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel, Params);

	if (bDrawDebug)
	{
		DrawDebugLine(World, Start, bHit ? OutHit.ImpactPoint : End, bHit ? FColor::Red : FColor::Green, false, 0.f, 0, 0.5f);
	}
	return bHit;
}

void ASnakeToy::ResetPath(const FVector& HeadPosition, const FVector& Up, const FVector& Forward)
{
	HeadPos = HeadPosition;
	HeadUp = Up.GetSafeNormal();
	HeadFwd = SnakeToyLocal::MakeTangent(Forward, HeadUp);
	FallSpeed = 0.f;
	AirTime = 0.f;

	// Corps posé en ligne droite derrière la tête
	float BodyLength = 0.f;
	for (int32 Index = 0; Index + 1 < NumSegments; ++Index)
	{
		BodyLength += SpacingRatio * (GetSegmentRadius(Index) + GetSegmentRadius(Index + 1));
	}

	Path.Reset();
	Path.Add({ HeadPos, HeadUp });
	Path.Add({ HeadPos - HeadFwd * (BodyLength + 100.f), HeadUp });
}

void ASnakeToy::PushPathPoint(const FVector& Position, const FVector& Up)
{
	if (Path.Num() > 0 && FVector::DistSquared(Path[0].Position, Position) < FMath::Square(SnakeToyLocal::PathStep))
	{
		Path[0] = { Position, Up };
	}
	else
	{
		Path.Insert({ Position, Up }, 0);
	}

	// On ne garde que la longueur utile au corps (+ marge)
	float Needed = 100.f;
	for (int32 Index = 0; Index + 1 < NumSegments; ++Index)
	{
		Needed += SpacingRatio * (GetSegmentRadius(Index) + GetSegmentRadius(Index + 1));
	}

	float Accumulated = 0.f;
	for (int32 Index = 1; Index < Path.Num(); ++Index)
	{
		Accumulated += FVector::Dist(Path[Index - 1].Position, Path[Index].Position);
		if (Accumulated > Needed && Index + 1 < Path.Num())
		{
			Path.SetNum(Index + 1);
			break;
		}
	}
}

bool ASnakeToy::SamplePath(float Distance, FVector& OutPosition, FVector& OutUp) const
{
	if (Path.Num() == 0)
	{
		return false;
	}
	if (Path.Num() == 1 || Distance <= 0.f)
	{
		OutPosition = Path[0].Position;
		OutUp = Path[0].Up;
		return true;
	}

	float Accumulated = 0.f;
	for (int32 Index = 1; Index < Path.Num(); ++Index)
	{
		const float SegLength = FVector::Dist(Path[Index - 1].Position, Path[Index].Position);
		if (Accumulated + SegLength >= Distance || Index == Path.Num() - 1)
		{
			const float Alpha = SegLength > KINDA_SMALL_NUMBER ? FMath::Clamp((Distance - Accumulated) / SegLength, 0.f, 1.f) : 0.f;
			OutPosition = FMath::Lerp(Path[Index - 1].Position, Path[Index].Position, Alpha);
			OutUp = FMath::Lerp(Path[Index - 1].Up, Path[Index].Up, Alpha).GetSafeNormal();
			return true;
		}
		Accumulated += SegLength;
	}
	return false;
}

void ASnakeToy::CrawlHead(float DeltaSeconds)
{
	using namespace SnakeToyLocal;

	const float R = HeadRadius;
	FHitResult Hit;

	// Errance : la tête tourne autour de sa normale selon un bruit lisse
	const float Turn = WanderStrength * 2.f * FMath::PerlinNoise1D(Time * WanderFrequency + 17.31f) * DeltaSeconds;
	HeadFwd = MakeTangent(HeadFwd.RotateAngleAxis(Turn, HeadUp), HeadUp);

	const float Step = Speed * DeltaSeconds;

	// 1. Un mur devant : on grimpe dessus (la nouvelle « avant » est l'ancien « haut »)
	if (FallSpeed <= 0.f && Trace(HeadPos, HeadPos + HeadFwd * (Step + R), Hit) && FVector::DotProduct(Hit.ImpactNormal, HeadUp) < 0.7f)
	{
		const FVector N = Hit.ImpactNormal;
		HeadFwd = MakeTangent(HeadUp, N);
		HeadUp = N;
		HeadPos = Hit.ImpactPoint + N * R;
		return;
	}

	const FVector Candidate = HeadPos + HeadFwd * Step;

	// 2. Le sol sous la tête : on s'y colle
	if (Trace(Candidate + HeadUp * R, Candidate - HeadUp * (R * 4.f + FallSpeed * DeltaSeconds), Hit))
	{
		const FVector N = Hit.ImpactNormal;
		HeadUp = (HeadUp + (N - HeadUp) * FMath::Min(1.f, DeltaSeconds * 12.f)).GetSafeNormal();
		HeadPos = Hit.ImpactPoint + N * R;
		HeadFwd = MakeTangent(HeadFwd, HeadUp);
		FallSpeed = 0.f;
		AirTime = 0.f;
		return;
	}

	// 3. Plus de sol : arête convexe, on passe de l'autre côté
	const FVector Below = Candidate - HeadUp * (R * 2.f);
	if (FallSpeed <= 0.f && Trace(Below, Below - HeadFwd * (R * 3.f), Hit))
	{
		const FVector N = Hit.ImpactNormal;
		HeadFwd = MakeTangent(-HeadUp, N);
		HeadUp = N;
		HeadPos = Hit.ImpactPoint + N * R;
		return;
	}

	// 4. Temps de grâce : une trace ratée isolée (rayon rasant, bord de triangle) ne fait pas tomber.
	//    On garde l'orientation et on continue tout droit quelques instants.
	if (FallSpeed <= 0.f && AirTime < CoyoteTime)
	{
		AirTime += DeltaSeconds;
		HeadPos = Candidate;
		return;
	}

	// 5. Rien autour : chute libre, la tête se remet droite
	FallSpeed += 980.f * DeltaSeconds;
	HeadPos = Candidate - FVector::UpVector * FallSpeed * DeltaSeconds;
	HeadUp = (HeadUp + (FVector::UpVector - HeadUp) * FMath::Min(1.f, DeltaSeconds * 3.f)).GetSafeNormal();
	HeadFwd = MakeTangent(HeadFwd, HeadUp);

	// Tombé dans le vide : retour au point de départ
	if (HeadPos.Z < GetActorLocation().Z - 100000.f)
	{
		ResetPath(GetActorLocation(), FVector::UpVector, GetActorForwardVector());
	}
}

void ASnakeToy::UpdateSegments()
{
	const int32 Count = Segments.Num();
	if (Count == 0)
	{
		return;
	}

	TArray<FVector> Positions;
	TArray<float> Radii;
	Positions.SetNum(Count);
	Radii.SetNum(Count);

	float Distance = 0.f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Radii[Index] = GetSegmentRadius(Index);
		if (Index > 0)
		{
			Distance += SpacingRatio * (Radii[Index - 1] + Radii[Index]);
		}

		FVector PathPos;
		FVector PathUp;
		if (!SamplePath(Distance, PathPos, PathUp))
		{
			PathPos = HeadPos;
			PathUp = HeadUp;
		}
		// Le chemin est celui du centre de la tête : les sphères plus petites descendent pour toucher le sol
		Positions[Index] = PathPos - PathUp * (HeadRadius - Radii[Index]);
	}

	TArray<float> Data;
	Data.SetNumZeroed(12);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UStaticMeshComponent* Comp = Segments[Index];
		Comp->SetWorldLocation(Positions[Index]);
		Comp->SetWorldScale3D(FVector(Radii[Index] / SnakeToyLocal::SphereMeshRadius));

		// Voisines relatives au centre courant (précision assurée loin de l'origine)
		for (int32 Slot = 0; Slot < 2; ++Slot)
		{
			const int32 Neighbor = Slot == 0 ? Index - 1 : Index + 1;
			const int32 Base = Slot * 4;
			if (Positions.IsValidIndex(Neighbor))
			{
				const FVector Offset = Positions[Neighbor] - Positions[Index];
				Data[Base + 0] = static_cast<float>(Offset.X);
				Data[Base + 1] = static_cast<float>(Offset.Y);
				Data[Base + 2] = static_cast<float>(Offset.Z);
				Data[Base + 3] = Radii[Neighbor];
			}
			else
			{
				Data[Base + 0] = Data[Base + 1] = Data[Base + 2] = Data[Base + 3] = 0.f;
			}
		}
		Data[8] = Radii[Index];
		Data[9] = BlendK;
		Data[10] = 0.f;
		Data[11] = 0.f;
		Comp->SetCustomPrimitiveDataFloatArray(0, Data);
	}

	if (bDrawDebug)
	{
		const UWorld* World = GetWorld();
		DrawDebugDirectionalArrow(World, HeadPos, HeadPos + HeadFwd * HeadRadius * 3.f, 10.f, FColor::Yellow, false, 0.f, 0, 1.f);
		DrawDebugLine(World, HeadPos, HeadPos + HeadUp * HeadRadius * 3.f, FColor::Cyan, false, 0.f, 0, 1.f);
	}
}

void ASnakeToy::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureSegments();

	// Aperçu en éditeur : corps en ligne derrière l'acteur
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		ResetPath(GetActorLocation(), GetActorUpVector(), GetActorForwardVector());
		UpdateSegments();
	}
}

void ASnakeToy::BeginPlay()
{
	Super::BeginPlay();

	EnsureSegments();

	// Départ posé sur le sol sous l'acteur, s'il y en a un
	FHitResult Hit;
	const FVector Start = GetActorLocation() + FVector::UpVector * HeadRadius;
	if (Trace(Start, Start - FVector::UpVector * 5000.f, Hit))
	{
		ResetPath(Hit.ImpactPoint + Hit.ImpactNormal * HeadRadius, Hit.ImpactNormal, GetActorForwardVector());
	}
	else
	{
		ResetPath(GetActorLocation(), FVector::UpVector, GetActorForwardVector());
	}
	UpdateSegments();
}

void ASnakeToy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Time += DeltaSeconds;
	CrawlHead(DeltaSeconds);
	PushPathPoint(HeadPos, HeadUp);
	UpdateSegments();
}
