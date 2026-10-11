#include "DwamLightComponent.h"

#include "Components/BillboardComponent.h"
#include "DwamLightSubsystem.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UDwamLightComponent::UDwamLightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	// Icône dans le viewport, comme une lumière d'Unreal
	bVisualizeComponent = true;
#endif
}

FLinearColor UDwamLightComponent::GetEmittedColor() const
{
	FLinearColor Color = LightColor;
	if (bUseTemperature)
	{
		Color *= FLinearColor::MakeFromColorTemperature(Temperature);
	}
	Color *= Intensity;
	Color.A = 1.f;
	return Color;
}

bool UDwamLightComponent::BuildLightData(FDwamLightData& Out) const
{
	if (!bEnabled || !GetVisibleFlag() || Intensity <= 0.f || Range <= 0.f)
	{
		return false;
	}

	if (const AActor* Owner = GetOwner())
	{
		const UWorld* World = GetWorld();
		if (World && World->IsGameWorld())
		{
			if (Owner->IsHidden() || bHiddenInGame)
			{
				return false;
			}
		}
#if WITH_EDITOR
		else if (Owner->IsTemporarilyHiddenInEditor(true))
		{
			return false;
		}
#endif
	}

	const FLinearColor Emitted = GetEmittedColor();
	Out.Position = GetComponentLocation();
	Out.Rotation = FQuat4f(GetComponentQuat());
	Out.Color = FVector3f(Emitted.R, Emitted.G, Emitted.B);
	Out.Range = Range;
	Out.FalloffExponent = FalloffExponent;
	Out.Type = Type;
	Out.Layers = (uint8)(Layers & 0xFF);
	Out.bCastShadow = bCastShadow;
	Out.ShadowSoftness = ShadowSoftness;

	const float Outer = FMath::Clamp(OuterConeAngle, 1.f, 89.f);
	const float Inner = FMath::Clamp(InnerConeAngle, 0.f, Outer);
	Out.CosOuter = FMath::Cos(FMath::DegreesToRadians(Outer));
	Out.CosInner = FMath::Cos(FMath::DegreesToRadians(Inner));
	Out.HalfSize = FVector2f(SourceWidth, SourceHeight) * 0.5f;
	return true;
}

void UDwamLightComponent::OnRegister()
{
	Super::OnRegister();
	UpdateSprite();

	if (UWorld* World = GetWorld())
	{
		if (UDwamLightSubsystem* Subsystem = World->GetSubsystem<UDwamLightSubsystem>())
		{
			Subsystem->RegisterLight(this);
		}
	}
}

void UDwamLightComponent::OnUnregister()
{
	if (UWorld* World = GetWorld())
	{
		if (UDwamLightSubsystem* Subsystem = World->GetSubsystem<UDwamLightSubsystem>())
		{
			Subsystem->UnregisterLight(this);
		}
	}
	Super::OnUnregister();
}

#if WITH_EDITOR
void UDwamLightComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateSprite();
}
#endif

void UDwamLightComponent::UpdateSprite()
{
#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		const TCHAR* Path = TEXT("/Engine/EditorResources/LightIcons/S_LightPoint.S_LightPoint");
		if (Type == EDwamLightType::Spot)
		{
			Path = TEXT("/Engine/EditorResources/LightIcons/S_LightSpot.S_LightSpot");
		}
		else if (Type == EDwamLightType::Rect)
		{
			Path = TEXT("/Engine/EditorResources/LightIcons/S_LightRect.S_LightRect");
		}
		SpriteComponent->SetSprite(LoadObject<UTexture2D>(nullptr, Path));
	}
#endif
}
