#include "DwamSunGlareComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/UObjectIterator.h"

UDwamSunGlareComponent::UDwamSunGlareComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Après la mise à jour du gameplay : la caméra de la frame est connue
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UDwamSunGlareComponent::BeginPlay()
{
	Super::BeginPlay();
	Glare = 0.f;
	WriteParameter(0.f);
}

void UDwamSunGlareComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Ne pas laisser l'image assombrie si le composant disparaît
	WriteParameter(0.f);
	Super::EndPlay(EndPlayReason);
}

UDirectionalLightComponent* UDwamSunGlareComponent::ResolveSun()
{
	if (SunActor)
	{
		UDirectionalLightComponent* Light = SunActor->FindComponentByClass<UDirectionalLightComponent>();
		CachedSun = Light;
		return Light;
	}

	if (CachedSun.IsValid())
	{
		return CachedSun.Get();
	}

	// Première lumière directionnelle de ce monde marquée « Atmosphere Sun Light »
	// (la même que celle qui alimente SkyAtmosphereLightDirection dans les matériaux)
	UWorld* World = GetWorld();
	for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
	{
		UDirectionalLightComponent* Light = *It;
		if (Light && Light->GetWorld() == World && Light->IsRegistered() && Light->IsUsedAsAtmosphereSunLight())
		{
			CachedSun = Light;
			return Light;
		}
	}
	return nullptr;
}

float UDwamSunGlareComponent::ComputeVisibility(const FVector& CameraLocation, const FVector& ToSun) const
{
	UWorld* World = GetWorld();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DwamSunGlare), bTraceComplex);
	Params.AddIgnoredActor(GetOwner());
	if (const APlayerController* PC = World->GetFirstPlayerController())
	{
		Params.AddIgnoredActor(PC->GetPawn());
	}

	// Centre + 4 directions écartées de ProbeSpread (comme les 5 lectures de profondeur de Wind Waker)
	FVector U, V;
	ToSun.FindBestAxisVectors(U, V);
	const float Spread = FMath::DegreesToRadians(ProbeSpread);
	const float C = FMath::Cos(Spread);
	const float S = FMath::Sin(Spread);
	const FVector Directions[5] =
	{
		ToSun,
		(ToSun * C + U * S).GetSafeNormal(),
		(ToSun * C - U * S).GetSafeNormal(),
		(ToSun * C + V * S).GetSafeNormal(),
		(ToSun * C - V * S).GetSafeNormal()
	};

	int32 NumFree = 0;
	for (const FVector& Direction : Directions)
	{
		const FVector End = CameraLocation + Direction * TraceDistance;
		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, CameraLocation, End, TraceChannel, Params);
		if (!bBlocked)
		{
			++NumFree;
		}

		if (bDebug)
		{
			DrawDebugLine(World, CameraLocation + Direction * 100.f, bBlocked ? Hit.ImpactPoint : End,
				bBlocked ? FColor::Red : FColor::Yellow, false, -1.f, 0, 1.f);
		}
	}
	return NumFree / 5.f;
}

void UDwamSunGlareComponent::WriteParameter(float Value)
{
	UWorld* World = GetWorld();
	if (!World || !ParameterCollection)
	{
		return;
	}

	if (UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(ParameterCollection))
	{
		const bool bFound = Instance->SetScalarParameterValue(ParameterName, Value);
		if (!bFound && !bWarnedMissingParameter)
		{
			bWarnedMissingParameter = true;
			UE_LOG(LogTemp, Warning, TEXT("Dwam Sun Glare : le paramètre scalaire « %s » n'existe pas dans %s."),
				*ParameterName.ToString(), *ParameterCollection->GetName());
		}
	}
}

void UDwamSunGlareComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Camera = PC ? PC->PlayerCameraManager.Get() : nullptr;
	const UDirectionalLightComponent* Sun = ResolveSun();

	float Target = 0.f;
	Stare = 0.f;
	Visibility = 0.f;

	if (Camera && Sun)
	{
		const FVector CameraLocation = Camera->GetCameraLocation();
		const FVector Forward = Camera->GetCameraRotation().Vector();
		const FVector ToSun = -Sun->GetDirection();   // GetDirection = sens de propagation de la lumière

		// Regard : 1 au centre, 0 à MaxAngle, courbe en puissance (Wind Waker : (1 - d / 450 px)^4)
		const float CosAngle = FMath::Clamp(FVector::DotProduct(Forward, ToSun), -1.f, 1.f);
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(CosAngle));
		const float Falloff = 1.f - FMath::Clamp(Angle / FMath::Max(MaxAngle, 1.f), 0.f, 1.f);
		Stare = FMath::Pow(Falloff, Power);

		// Visibilité : seulement si on regarde vers le soleil (économise les traces)
		if (Stare > 0.f)
		{
			Visibility = ComputeVisibility(CameraLocation, ToSun);
		}
		Target = Stare * Visibility * Strength;
	}

	const float Speed = Target > Glare ? RiseSpeed : FallSpeed;
	Glare = FMath::FInterpTo(Glare, Target, DeltaTime, Speed);
	WriteParameter(Glare);

	if (bDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(reinterpret_cast<uint64>(this), 0.f, FColor::Yellow,
			FString::Printf(TEXT("Sun Glare : regard %.2f  visibilite %.2f  eblouissement %.2f%s"),
				Stare, Visibility, Glare, Sun ? TEXT("") : TEXT("  (aucun soleil trouve)")));
	}
}
