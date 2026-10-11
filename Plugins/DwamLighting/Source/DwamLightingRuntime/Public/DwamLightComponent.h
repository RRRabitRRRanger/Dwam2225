#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DwamLightTypes.h"
#include "DwamLightComponent.generated.h"

/**
 * Lumière du pipeline d'éclairage maison.
 *
 * Ce n'est pas une lumière d'Unreal : le moteur ne la voit pas. Elle s'inscrit auprès du
 * UDwamLightSubsystem, qui l'envoie aux matériaux par la texture des lumières (Lights.ush).
 *
 * Intensité : 1 = pleine lumière au centre, puis fondu jusqu'à 0 à la portée :
 *   éclairement = Intensity × (1 - (distance / Range)²) ^ FalloffExponent
 */
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent, DisplayName = "Dwam Light"),
	HideCategories = (Physics, Collision, Navigation, Cooking, HLOD, Mobile, AssetUserData))
class DWAMLIGHTINGRUNTIME_API UDwamLightComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDwamLightComponent();

	// ---------------------------------------------------------------- Lumière

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	EDwamLightType Type = EDwamLightType::Point;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light")
	FLinearColor LightColor = FLinearColor::White;

	/** Multiplie la couleur par celle d'un corps noir à cette température. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (InlineEditConditionToggle))
	bool bUseTemperature = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light", meta = (EditCondition = "bUseTemperature", ClampMin = "1000", ClampMax = "15000", UIMin = "1700", UIMax = "12000"))
	float Temperature = 6500.f;

	/** 1 = pleine lumière au centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light", meta = (ClampMin = "0", UIMax = "8"))
	float Intensity = 1.f;

	/** Distance à laquelle la lumière s'éteint complètement (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light", meta = (ClampMin = "1", UIMax = "5000", Units = "Centimeters"))
	float Range = 1000.f;

	/** Dureté du fondu : petit = lumière large qui s'éteint tard, grand = lumière serrée autour du centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light", meta = (ClampMin = "0.0625", ClampMax = "15.9", UIMin = "0.25", UIMax = "8"))
	float FalloffExponent = 2.f;

	/** Les matériaux dont le masque de layers ne croise pas celui-ci ne sont pas éclairés. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (Bitmask, BitmaskEnum = "/Script/DwamLightingRuntime.EDwamLightLayer"))
	int32 Layers = 1;

	// ---------------------------------------------------------------- Spot

	/** Demi-angle du cône de pleine lumière (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light|Spot", meta = (EditCondition = "Type == EDwamLightType::Spot", ClampMin = "0", ClampMax = "89", Units = "Degrees"))
	float InnerConeAngle = 20.f;

	/** Demi-angle du cône au-delà duquel il n'y a plus de lumière (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light|Spot", meta = (EditCondition = "Type == EDwamLightType::Spot", ClampMin = "1", ClampMax = "89", Units = "Degrees"))
	float OuterConeAngle = 40.f;

	// ---------------------------------------------------------------- Rect

	/** Largeur du rectangle émetteur, le long de l'axe Y (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light|Rect", meta = (EditCondition = "Type == EDwamLightType::Rect", ClampMin = "1", Units = "Centimeters"))
	float SourceWidth = 100.f;

	/** Hauteur du rectangle émetteur, le long de l'axe Z (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Light|Rect", meta = (EditCondition = "Type == EDwamLightType::Rect", ClampMin = "1", Units = "Centimeters"))
	float SourceHeight = 100.f;

	// ---------------------------------------------------------------- Ombre (réservé)

	/** Transmis au shader (drapeau) ; l'ombre par lumière n'est pas encore branchée. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Shadow")
	bool bCastShadow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Shadow", meta = (EditCondition = "bCastShadow", ClampMin = "0", ClampMax = "1"))
	float ShadowSoftness = 0.5f;

	// ----------------------------------------------------------------

	/** Couleur émise : RGB linéaire, température et intensité comprises. */
	UFUNCTION(BlueprintPure, Category = "Dwam Light")
	FLinearColor GetEmittedColor() const;

	/** Remplit Out et renvoie vrai si la lumière éclaire quelque chose cette frame. */
	bool BuildLightData(FDwamLightData& Out) const;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void UpdateSprite();
};
