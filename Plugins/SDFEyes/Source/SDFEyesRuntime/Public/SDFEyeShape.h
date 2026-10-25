#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SDFEyeShape.generated.h"

/**
 * Forme de paupière : les réglages de FSDFEyeShape (Eyelids.ush), mêmes noms que le JSON de la démo.
 * Un asset par type d'œil (humain, orc…), réutilisé par tous les yeux qui le portent. Défauts de l'auteur (2026-10-17).
 */
USTRUCT(BlueprintType)
struct SDFEYESRUNTIME_API FSDFEyeShapeSettings
{
	GENERATED_BODY()

	/** Hauteur du sommet du bord supérieur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upper") float EU = 0.22f;
	/** Position du sommet : -1 nez .. +1 tempe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upper") float TU = 0.24f;
	/** Forme du bord supérieur : < 1 ronde, > 1 pointue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upper") float PU = 0.93f;
	/** Hauteur du creux du bord inférieur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lower") float EL = -0.22f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lower") float TL = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lower") float PL = 1.f;
	/** Demi-écart des coins (azimut, radians). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corners") float Width = 0.62f;
	/** Inclinaison des coins : l'humeur du regard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corners") float Tilt = 0.1f;
	/** Ligne de fermeture : 0 bas .. 1 haut. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corners") float Close = 0.47f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fold") float FoldGap = 0.23f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fold") float Fold = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fold") float Hood = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lid") float Thick = 0.11f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lashes") float Lash = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lashes") float Curl = 0.32f;
	/** Chair du coin interne (caroncule). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Globe") float Car = 0.93f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Globe") float Veins = 1.f;
};

UCLASS(BlueprintType)
class SDFEYESRUNTIME_API USDFEyeShape : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ShowOnlyInnerProperties))
	FSDFEyeShapeSettings Settings;
};
