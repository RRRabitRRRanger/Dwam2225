#pragma once

#include "CoreMinimal.h"
#include "DwamLightTypes.h"
#include "RHIResources.h"
#include "SceneViewExtension.h"

/**
 * Côté render thread du système de lumières : juste avant le rendu d'une view family, empaquette les
 * lumières relativement à la caméra de ce rendu et les écrit dans le render target.
 * Une instance par monde (FWorldSceneViewExtension) : l'éditeur et le PIE partagent le même asset,
 * chacun y écrit ses lumières juste avant de s'en servir.
 */
class FDwamLightViewExtension : public FWorldSceneViewExtension
{
public:
	FDwamLightViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld);

	/** Remplace les données de la frame. Texture nulle : plus rien n'est écrit. */
	void SetFrameData_RenderThread(TArray<FDwamLightData>&& InLights, FTextureRHIRef InTexture, int32 InMaxLights);

	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;

private:
	TArray<FDwamLightData> Lights;
	FTextureRHIRef Texture;
	int32 MaxLights = 0;
	bool bWarnedFormat = false;
	bool bWarnedBudget = false;
};
