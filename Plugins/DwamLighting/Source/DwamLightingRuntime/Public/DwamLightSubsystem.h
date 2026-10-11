#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DwamLightSubsystem.generated.h"

class FDwamLightViewExtension;
class UDwamLightComponent;
class UTextureRenderTarget2D;

/**
 * Collecte les lumières maison d'un monde et les confie au render thread.
 *
 * Game thread (ici) : la liste des UDwamLightComponent inscrits, relue à chaque frame.
 * Render thread (FDwamLightViewExtension) : juste avant chaque rendu, les lumières sont empaquetées
 * relativement à la caméra de CE rendu et écrites dans le render target lu par Lights.ush.
 * Tourne aussi dans l'éditeur, sans lancer le jeu.
 */
UCLASS()
class DWAMLIGHTINGRUNTIME_API UDwamLightSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return true; }

	void RegisterLight(UDwamLightComponent* Light);
	void UnregisterLight(UDwamLightComponent* Light);

	/** Lumières inscrites dans ce monde (allumées ou non). */
	UFUNCTION(BlueprintPure, Category = "Dwam Light")
	int32 GetNumRegisteredLights() const { return Lights.Num(); }

	/** Lumières envoyées au rendu à la dernière frame (avant le budget). */
	UFUNCTION(BlueprintPure, Category = "Dwam Light")
	int32 GetNumActiveLights() const { return NumActiveLights; }

private:
	void ResolveRenderTarget();
	void PushToRenderThread(TArray<struct FDwamLightData>&& Data);

	TArray<TWeakObjectPtr<UDwamLightComponent>> Lights;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	TSharedPtr<FDwamLightViewExtension, ESPMode::ThreadSafe> ViewExtension;

	int32 NumActiveLights = 0;
	/** Secondes avant de rechercher à nouveau un render target manquant. */
	float ResolveCooldown = 0.f;
};
