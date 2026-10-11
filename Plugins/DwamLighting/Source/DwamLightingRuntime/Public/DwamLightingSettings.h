#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DwamLightingSettings.generated.h"

class UTextureRenderTarget2D;

/** Réglages du pipeline d'éclairage maison : Project Settings > Plugins > DWAM Lighting. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "DWAM Lighting"))
class DWAMLIGHTINGRUNTIME_API UDwamLightingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDwamLightingSettings();

	/**
	 * Render target qui porte les lumières jusqu'aux matériaux.
	 * À créer dans l'éditeur : format RTF RGBA32f, hauteur 1, largeur = 2 × (lumières + 1), ex. 256 × 1 pour 127 lumières.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Lights")
	TSoftObjectPtr<UTextureRenderTarget2D> LightsRenderTarget;

	/** Budget de lumières par image. Au-delà, seules les plus importantes pour la caméra sont gardées. */
	UPROPERTY(Config, EditAnywhere, Category = "Lights", meta = (ClampMin = "1", ClampMax = "1023"))
	int32 MaxLights = 64;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
