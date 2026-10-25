#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "CharacterPreset.h"
#include "CharacterKitAnimInstance.generated.h"

class UAnimSequence;

/** Côté thread d'animation : joue la séquence puis applique le profil de corps. */
struct FCharacterKitAnimProxy : public FAnimInstanceProxy
{
	FCharacterKitAnimProxy() = default;
	explicit FCharacterKitAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	TObjectPtr<const UAnimSequence> Sequence;
	double Time = 0.0;
	FCharacterBodyProfile Profile;
	float RefLegHeight = 0.f;
};

/**
 * Anim instance minimale sans Blueprint : une séquence en boucle + le profil de corps (longueur des os).
 * Sert aux tests et aux aperçus (outil de personnage). En jeu, le profil passera par un nœud d'AnimBP.
 */
UCLASS(Transient, NotBlueprintable)
class CHARACTERKIT_API UCharacterKitAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Sequence;

	/** Temps dans la séquence (s). Avance tout seul au tick, ou se règle à la main (tests). */
	double Time = 0.0;

	FCharacterBodyProfile Profile;

	void SetProfile(const FCharacterBodyProfile& InProfile) { Profile = InProfile; }

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FCharacterKitAnimProxy(this); }

private:
	friend struct FCharacterKitAnimProxy;
	float RefLegHeight = 0.f;
};
