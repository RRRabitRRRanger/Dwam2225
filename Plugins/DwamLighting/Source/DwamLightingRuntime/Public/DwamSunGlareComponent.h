#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "DwamSunGlareComponent.generated.h"

class UDirectionalLightComponent;
class UMaterialParameterCollection;

/**
 * Éblouissement solaire façon Wind Waker (d_kankyo_rain.cpp, décompilation zeldaret/tww).
 *
 * Chaque frame, côté CPU :
 *  - regard   = (1 - angle(caméra, soleil) / MaxAngle)^Power  (Wind Waker : distance écran / 450 px, puissance 4) ;
 *  - visibilité = part des 5 traces vers le soleil qui ne touchent rien (Wind Waker : 5 lectures du Z-buffer) ;
 *  - éblouissement = regard × visibilité × Strength, lissé dans le temps ;
 *  - écrit dans un paramètre scalaire de la Material Parameter Collection du post-process.
 * Le matériau de post-process l'applique avec DwamApplySunGlare (PostProcess.ush) :
 * objets assombris, ciel éclairci.
 *
 * À poser sur n'importe quel acteur présent en jeu (Player Controller, Pawn, Game State...).
 * Utilise la caméra du premier joueur local.
 */
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent, DisplayName = "Dwam Sun Glare"))
class DWAMLIGHTINGRUNTIME_API UDwamSunGlareComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDwamSunGlareComponent();

	// ---------------------------------------------------------------- Sortie

	/** Collection du post-process (ex. MPC_DwamPost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare")
	TObjectPtr<UMaterialParameterCollection> ParameterCollection;

	/** Paramètre scalaire écrit dans la collection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare")
	FName ParameterName = TEXT("SunGlare");

	/** Multiplicateur global (0 = effet coupé, ex. option du menu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare", meta = (ClampMin = "0", ClampMax = "1"))
	float Strength = 1.f;

	// ---------------------------------------------------------------- Soleil et regard

	/** Acteur portant la lumière directionnelle du soleil. Vide : première lumière directionnelle marquée « Atmosphere Sun Light ». */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Sun")
	TObjectPtr<AActor> SunActor;

	/** Angle entre l'axe de la caméra et le soleil au-delà duquel l'effet est nul (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Sun", meta = (ClampMin = "1", ClampMax = "90", Units = "Degrees"))
	float MaxAngle = 40.f;

	/** Puissance de la courbe du regard (Wind Waker : 4). Plus elle est forte, plus l'effet se concentre au centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Sun", meta = (ClampMin = "0.1", ClampMax = "16"))
	float Power = 4.f;

	// ---------------------------------------------------------------- Visibilité

	/** Écart angulaire des 4 traces autour de la trace centrale (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Visibility", meta = (ClampMin = "0", ClampMax = "10", Units = "Degrees"))
	float ProbeSpread = 1.f;

	/** Longueur des traces (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Visibility", meta = (ClampMin = "100", Units = "Centimeters"))
	float TraceDistance = 200000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Visibility")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Traces complexes (géométrie visuelle) : plus juste pour un feuillage ou une arche. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Visibility")
	bool bTraceComplex = true;

	// ---------------------------------------------------------------- Lissage

	/** Vitesse de montée de l'éblouissement (1/s, interpolation exponentielle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Smoothing", meta = (ClampMin = "0.1"))
	float RiseSpeed = 2.f;

	/** Vitesse de descente (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Smoothing", meta = (ClampMin = "0.1"))
	float FallSpeed = 4.f;

	// ---------------------------------------------------------------- État (lecture)

	/** Éblouissement lissé, tel qu'écrit dans la collection. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Sun Glare|State")
	float Glare = 0.f;

	/** Regard brut 0..1 (sans visibilité ni lissage). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Sun Glare|State")
	float Stare = 0.f;

	/** Visibilité brute 0..1 (part des traces libres). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Sun Glare|State")
	float Visibility = 0.f;

	/** Dessine les traces et affiche les valeurs à l'écran. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Glare|Debug")
	bool bDebug = false;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Lumière du soleil (trouvée une fois, puis gardée). */
	TWeakObjectPtr<UDirectionalLightComponent> CachedSun;

	bool bWarnedMissingParameter = false;

	UDirectionalLightComponent* ResolveSun();
	float ComputeVisibility(const FVector& CameraLocation, const FVector& ToSun) const;
	void WriteParameter(float Value);
};
