#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GPUTraceTypes.h"
#include "GPUTraceBugAgent.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** État de l'agent Bug. */
UENUM(BlueprintType)
enum class EGPUTraceBugMode : uint8
{
	Idle		UMETA(DisplayName = "Inactif"),
	GoToGoal	UMETA(DisplayName = "Vers le but"),
	FollowWall	UMETA(DisplayName = "Longe la paroi"),
	Reached		UMETA(DisplayName = "Arrivé"),
	Stuck		UMETA(DisplayName = "Bloqué"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPUTraceBugModeChanged, EGPUTraceBugMode, NewMode);

/**
 * Démo GPUTrace : un agent qui rejoint un but en ne « voyant » le monde que par des traces
 * sur la géométrie visuelle (algorithme Bug de la robotique, adapté à la 3D).
 *
 *  - Vers le but : il avance en ligne droite tant que le rayon vers le but est libre.
 *  - Obstacle : il longe la paroi (palpe vers la paroi, puis tout droit, puis s'en écarte).
 *  - Il repart vers le but dès que la voie est libre ET qu'il est plus proche du but qu'au point
 *    de contact (critère de Bug2). En 3D, c'est une adaptation heuristique, pas une preuve de convergence.
 *
 * Il se déplace sans physique : sa boîte de collision ne le bloque pas, seules les traces comptent.
 * Scène de test : le placer dans un tuyau dont la collision simple est une boîte pleine, but à l'extérieur.
 */
UCLASS(meta = (DisplayName = "GPU Trace Bug Agent"))
class GPUTRACE_API AGPUTraceBugAgent : public AActor
{
	GENERATED_BODY()

public:
	AGPUTraceBugAgent();

	/** But, relatif à la position de départ de l'agent (handle déplaçable dans le viewport). Ignoré si GoalActor est défini. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug", meta = (MakeEditWidget))
	FVector GoalOffset = FVector(1000.f, 0.f, 0.f);

	/** But mobile (optionnel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug")
	TObjectPtr<AActor> GoalActor;

	/** Distance parcourue à chaque cycle (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug", meta = (ClampMin = "1", Units = "cm"))
	float StepSize = 10.f;

	/** Rayon de l'agent : distance gardée avec les parois (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug", meta = (ClampMin = "1", Units = "cm"))
	float AgentRadius = 20.f;

	/** Côté préféré pour contourner un obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug")
	bool bFollowRight = true;

	/** Démarre tout seul au lancement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug")
	bool bAutoStart = true;

	/** Errance : une fois arrivé (ou bloqué), choisit un nouveau but au hasard et continue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug")
	bool bWander = false;

	/** Rayon dans lequel l'errance choisit ses buts (cm, autour de la position actuelle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug", meta = (ClampMin = "100", Units = "cm", EditCondition = "bWander"))
	float WanderRadius = 1500.f;

	/** Abandon après ce nombre de cycles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bug", meta = (ClampMin = "1"))
	int32 MaxCycles = 3000;

	/** Messages à l'écran (Print String) à chaque changement d'état. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bPrintMessages = true;

	/** Dessine les rayons, le chemin parcouru et le but. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebug = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Bug")
	EGPUTraceBugMode Mode = EGPUTraceBugMode::Idle;

	UPROPERTY(BlueprintAssignable, Category = "Bug")
	FGPUTraceBugModeChanged OnModeChanged;

	UFUNCTION(BlueprintCallable, Category = "Bug")
	void StartBug();

	UFUNCTION(BlueprintCallable, Category = "Bug")
	void StopBug();

	/** Relance depuis la position actuelle (bouton dans Details, utilisable pendant le jeu). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Bug", meta = (DisplayName = "Relancer"))
	void Restart();

	UFUNCTION(BlueprintPure, Category = "Bug")
	FVector GetGoalLocation() const;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FBatch
	{
		int32 Serial = 0;
		TArray<FVector> Directions;
		TArray<float> Lengths;
		TArray<FGPUTraceResult> Results;
		int32 Remaining = 0;
	};

	void Think();
	void OnBatchDone();
	void SetMode(EGPUTraceBugMode NewMode, const FString& Reason);
	void Say(const FString& Text, const FColor& Color) const;
	void MoveTo(const FVector& NewLocation);
	static bool IsFree(const FGPUTraceResult& Result, float Length);

	UPROPERTY(VisibleAnywhere, Category = "Bug")
	TObjectPtr<UBoxComponent> TrapBox;

	UPROPERTY(VisibleAnywhere, Category = "Bug")
	TObjectPtr<UStaticMeshComponent> Body;

	TSharedPtr<FBatch> Batch;
	uint64 BatchStartFrame = 0;
	bool bHasGoalOverride = false;
	FVector GoalOverride = FVector::ZeroVector;
	void PickWanderGoal();
	int32 BatchSerial = 0;
	int32 Cycles = 0;
	FTransform StartTransform = FTransform::Identity;
	FVector WallNormal = FVector::UpVector;
	FVector FollowDir = FVector::ForwardVector;
	float HitDistToGoal = 0.f;
	float LastWallDistance = 0.f;
	bool bWallSeen = false;
	bool bTriedReverse = false;
	TArray<FVector> Trail;
};
