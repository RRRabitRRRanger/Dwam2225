#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCollision.h"
#include "NiagaraDataInterfaceExport.h"
#include "GPUTraceTypes.h"
#include "GPUTraceSubsystem.generated.h"

class UNiagaraComponent;
class UGPUTraceSubsystem;

/** Reçoit les résultats exportés par le système Niagara (callback du data interface Export). */
UCLASS()
class GPUTRACE_API UGPUTraceExportHandler : public UObject, public INiagaraParticleCallbackHandler
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UGPUTraceSubsystem> Owner;

	virtual void ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* NiagaraSystem, const FVector& SimulationPositionOffset) override;
};

/**
 * Service de traces « visuelles », asynchrones, accessible depuis le Blueprint et le C++.
 *
 * Deux moteurs derrière la même API :
 *  - CPU : traces asynchrones d'Unreal, en mode complexe (triangles réels). Résultat à la frame suivante.
 *  - GPU : un système Niagara persistant (Async GPU Trace). Résultat 2 à 3 frames plus tard.
 *    Sans système configuré, repli automatique sur le CPU.
 *
 * Debug : paramètre bDrawDebug de chaque requête, ou variable console GPUTrace.Debug 1 pour tout afficher.
 */
UCLASS()
class GPUTRACE_API UGPUTraceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Demande une trace de Start à End. Le résultat arrive plus tard dans OnResult.
	 * @return l'identifiant de la requête (aussi présent dans le résultat)
	 */
	UFUNCTION(BlueprintCallable, Category = "GPUTrace")
	int32 RequestTrace(FVector Start, FVector End, FGPUTraceResultDelegate OnResult, bool bDrawDebug = false);

	/** Version C++ : le callback est une fonction quelconque. IgnoredActor n'est pris en compte qu'en CPU. */
	int32 RequestTraceNative(const FVector& Start, const FVector& End, TFunction<void(const FGPUTraceResult&)> Callback,
		bool bDrawDebug = false, const AActor* IgnoredActor = nullptr);

	/** Choisit le moteur. En GPU sans système configuré, les traces repassent sur le CPU. */
	UFUNCTION(BlueprintCallable, Category = "GPUTrace")
	void SetBackend(EGPUTraceBackend NewBackend);

	/** Moteur demandé. */
	UFUNCTION(BlueprintPure, Category = "GPUTrace")
	EGPUTraceBackend GetBackend() const { return Backend; }

	/** Moteur réellement utilisé (après un éventuel repli). */
	UFUNCTION(BlueprintPure, Category = "GPUTrace")
	EGPUTraceBackend GetActiveBackend() const;

	/** Nombre de traces en attente de résultat. */
	UFUNCTION(BlueprintPure, Category = "GPUTrace")
	int32 GetPendingCount() const { return Pending.Num(); }

	// UTickableWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Appelé par UGPUTraceExportHandler sur le thread de jeu. */
	void HandleNiagaraResults(const TArray<FBasicParticleData>& Data, const FVector& PositionOffset);

private:
	struct FPendingTrace
	{
		int32 Id = 0;
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		TFunction<void(const FGPUTraceResult&)> Callback;
		bool bDrawDebug = false;
		uint64 RequestFrame = 0;
		TWeakObjectPtr<const AActor> IgnoredActor;
		EGPUTraceBackend Backend = EGPUTraceBackend::CPU;
		int32 Slot = INDEX_NONE;
	};

	void DispatchCPU(FPendingTrace& Trace);
	void OnCPUTraceDone(const FTraceHandle& Handle, FTraceDatum& Datum);

	bool EnsureNiagara();
	void DispatchNiagara();
	void CheckNiagaraTimeouts();

	void Complete(int32 Id, FGPUTraceResult& Result);
	void DrawResult(const FGPUTraceResult& Result) const;

	EGPUTraceBackend Backend = EGPUTraceBackend::CPU;
	int32 NextId = 1;

	TMap<int32, FPendingTrace> Pending;
	/** Requêtes GPU en attente d'un slot libre. */
	TArray<int32> NiagaraQueue;

	FTraceDelegate CPUTraceDelegate;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGPUTraceExportHandler> ExportHandler;

	/** Par slot : identifiant de la requête en cours (INDEX_NONE = libre). */
	TArray<int32> SlotRequest;
	/** Par slot : dernier identifiant envoyé au GPU (le GPU ne lance une trace que si l'identifiant augmente). */
	TArray<int32> SlotIds;
	TArray<FVector> SlotStarts;
	TArray<FVector> SlotEnds;
	bool bSlotsDirty = false;
	bool bNiagaraUnavailableLogged = false;
};
