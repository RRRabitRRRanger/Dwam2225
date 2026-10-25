#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "SDFEyeShape.h"
#include "SDFEyeComponent.generated.h"

class UMaterialInstanceDynamic;

/**
 * Un œil SDFEyes : une petite sphère invisible en soi, dont le matériau dessine le globe, les paupières
 * et les cils par ray-march (Eyelids.ush). Le regard au repos suit le X local, le haut le Z local.
 * Un œil = un component, posé n'importe où (ancres de CharacterKit, ou à la main).
 * Micro-vie : clignements tous les 2 à 6 s avec rebond, micro-saccades (Plan_Eyes).
 */
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent))
class SDFEYESRUNTIME_API USDFEyeComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	USDFEyeComponent();

	/** Forme partagée (asset). Vide = ShapeOverride. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye")
	TObjectPtr<USDFEyeShape> Shape;

	/** Forme propre à cet œil, quand Shape est vide. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye")
	FSDFEyeShapeSettings ShapeOverride;

	/** Matériau des yeux (nœud Custom qui inclut /Plugin/SDFEyes/Eyelids.ush). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye")
	TObjectPtr<UMaterialInterface> EyeMaterial;

	/** Rayon du globe, en cm. La sphère support fait 1,2 fois ce rayon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye", meta = (ClampMin = "0.1"))
	float GlobeRadius = 1.25f;

	/** Œil de l'autre côté : le nez passe de l'autre côté. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye")
	bool bMirror = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye|Animation", meta = (ClampMin = "0", ClampMax = "1"))
	float Blink = 0.f;

	/** Regard (radians) : X vers le Y local de l'œil (même sens pour tous les yeux, miroir compris), Y haut / bas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye|Animation")
	FVector2D Gaze = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye|Animation", meta = (ClampMin = "0", ClampMax = "1"))
	float Follow = 0.6f;

	/** Rayon angulaire de la pupille (radians). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye|Animation")
	float Pupil = 0.13f;

	/** Clignements et micro-saccades automatiques. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eye|Animation")
	bool bMicroLife = true;

	/** Applique forme, animation et taille au matériau. Appelé tout seul au tick et à l'enregistrement. */
	UFUNCTION(BlueprintCallable, Category = "Eye")
	void UpdateEye();

	/** Temps de micro-vie (s), pour piloter l'œil de l'extérieur (tests, cinématiques). */
	void SetMicroLifeTime(double InTime) { MicroTime = InTime; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnRegister() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void EvaluateMicroLife(float& OutBlink, FVector2D& OutGaze) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID;

	double MicroTime = 0.0;
	int32 Seed = 0;
};
