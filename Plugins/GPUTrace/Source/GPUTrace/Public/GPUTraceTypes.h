#pragma once

#include "CoreMinimal.h"
#include "GPUTraceTypes.generated.h"

/** Moteur utilisé pour résoudre les traces. */
UENUM(BlueprintType)
enum class EGPUTraceBackend : uint8
{
	/** Traces asynchrones d'Unreal en mode complexe (triangles réels des meshes). Aucun asset requis. */
	CPU		UMETA(DisplayName = "CPU (traces complexes)"),
	/** Système Niagara (Async GPU Trace). Nécessite le système décrit dans Docs/Guide_GPUTrace.md. */
	Niagara	UMETA(DisplayName = "GPU (Niagara)"),
};

/** Résultat d'une trace. */
USTRUCT(BlueprintType)
struct GPUTRACE_API FGPUTraceResult
{
	GENERATED_BODY()

	/** Identifiant renvoyé par la requête. */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	int32 RequestId = -1;

	/** Vrai si le rayon a touché quelque chose. */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	bool bHit = false;

	/** Vrai si le résultat n'est jamais revenu à temps (compté comme raté). */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	bool bTimedOut = false;

	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	FVector End = FVector::ZeroVector;

	/** Point d'impact (ou End si raté). */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	FVector Location = FVector::ZeroVector;

	/** Normale à l'impact (zéro si raté). */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	FVector Normal = FVector::ZeroVector;

	/** Distance du départ à l'impact (ou longueur du rayon si raté). */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	float Distance = 0.f;

	/** Acteur touché (moteur CPU seulement ; toujours vide en GPU). */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	TObjectPtr<AActor> HitActor = nullptr;

	/** Moteur qui a résolu la trace. */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	EGPUTraceBackend Backend = EGPUTraceBackend::CPU;

	/** Nombre de frames entre la requête et le résultat. */
	UPROPERTY(BlueprintReadOnly, Category = "GPUTrace")
	int32 LatencyFrames = 0;
};

/** Retour d'une trace (Blueprint : événement à brancher sur « Request Trace »). */
DECLARE_DYNAMIC_DELEGATE_OneParam(FGPUTraceResultDelegate, const FGPUTraceResult&, Result);
