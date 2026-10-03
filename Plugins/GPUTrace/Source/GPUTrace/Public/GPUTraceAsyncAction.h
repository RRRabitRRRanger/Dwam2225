#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GPUTraceTypes.h"
#include "GPUTraceAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPUTraceAsyncPin, const FGPUTraceResult&, Result);

/** Nœud Blueprint « Async GPU Trace » : lance une trace et continue sur On Hit ou On Miss quand le résultat arrive. */
UCLASS()
class GPUTRACE_API UGPUTraceAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Le rayon a touché quelque chose. */
	UPROPERTY(BlueprintAssignable)
	FGPUTraceAsyncPin OnHit;

	/** Le rayon n'a rien touché (ou le résultat n'est pas revenu à temps : voir bTimedOut). */
	UPROPERTY(BlueprintAssignable)
	FGPUTraceAsyncPin OnMiss;

	/**
	 * Trace sur la géométrie visuelle, avec le moteur actif du sous-système GPUTrace.
	 * Le résultat arrive quelques frames plus tard (1 en CPU, 2 à 3 en GPU).
	 */
	UFUNCTION(BlueprintCallable, Category = "GPUTrace", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Async GPU Trace"))
	static UGPUTraceAsyncAction* AsyncGPUTrace(UObject* WorldContextObject, FVector Start, FVector End, bool bDrawDebug = true);

	virtual void Activate() override;

private:
	TWeakObjectPtr<UObject> WorldContext;
	FVector TraceStart = FVector::ZeroVector;
	FVector TraceEnd = FVector::ZeroVector;
	bool bDebug = true;
};
