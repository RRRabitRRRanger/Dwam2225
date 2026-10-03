#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "GPUTraceTypes.h"
#include "GPUTraceSettings.generated.h"

class UNiagaraSystem;

/** Réglages du projet : Project Settings > Plugins > GPU Trace. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GPU Trace"))
class GPUTRACE_API UGPUTraceSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Moteur utilisé au démarrage (modifiable ensuite par « Set Backend »). */
	UPROPERTY(Config, EditAnywhere, Category = "Général")
	EGPUTraceBackend DefaultBackend = EGPUTraceBackend::CPU;

	/** Système Niagara du moteur GPU (voir Docs/Guide_GPUTrace.md). Vide = repli sur le CPU. */
	UPROPERTY(Config, EditAnywhere, Category = "GPU (Niagara)")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	/** Nombre de rayons simultanés côté GPU (= nombre de particules du système, = taille des tableaux). */
	UPROPERTY(Config, EditAnywhere, Category = "GPU (Niagara)", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 NiagaraSlots = 256;

	/** Au-delà de ce nombre de frames sans réponse, une trace GPU est déclarée ratée (bTimedOut). */
	UPROPERTY(Config, EditAnywhere, Category = "GPU (Niagara)", meta = (ClampMin = "2"))
	int32 NiagaraTimeoutFrames = 30;

	/** Canal de collision des traces CPU. */
	UPROPERTY(Config, EditAnywhere, Category = "CPU")
	TEnumAsByte<ECollisionChannel> CPUTraceChannel = ECC_Visibility;

	/** Traces CPU sur les triangles réels (géométrie visuelle) plutôt que sur les formes simples. */
	UPROPERTY(Config, EditAnywhere, Category = "CPU")
	bool bCPUTraceComplex = true;

	/** Durée d'affichage des traces de debug (secondes). */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (ClampMin = "0"))
	float DebugDrawDuration = 1.f;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
