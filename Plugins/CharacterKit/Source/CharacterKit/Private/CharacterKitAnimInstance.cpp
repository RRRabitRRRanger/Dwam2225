#include "CharacterKitAnimInstance.h"
#include "CharacterBodyProfile.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AttributesRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

void UCharacterKitAnimInstance::NativeInitializeAnimation()
{
	// Hauteur bassin → chevilles en pose de référence : sert à garder les pieds au sol quand les jambes changent
	RefLegHeight = 0.f;
	const USkeletalMeshComponent* Skel = GetSkelMeshComponent();
	if (Skel && Skel->GetSkeletalMeshAsset())
	{
		const FReferenceSkeleton& Ref = Skel->GetSkeletalMeshAsset()->GetRefSkeleton();
		const TArray<FTransform>& Local = Ref.GetRefBonePose();
		auto ComponentZ = [&](const TCHAR* Name)
		{
			int32 I = Ref.FindBoneIndex(Name);
			FTransform T = FTransform::Identity;
			while (I != INDEX_NONE)
			{
				T = T * Local[I];
				I = Ref.GetParentIndex(I);
			}
			return T.GetLocation().Z;
		};
		RefLegHeight = ComponentZ(TEXT("pelvis")) - ComponentZ(TEXT("foot_l"));
	}
}

void UCharacterKitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Time += DeltaSeconds;
}

void FCharacterKitAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	const UCharacterKitAnimInstance* Instance = CastChecked<UCharacterKitAnimInstance>(InAnimInstance);
	Sequence = Instance->Sequence;
	Time = Instance->Time;
	Profile = Instance->Profile;
	RefLegHeight = Instance->RefLegHeight;
}

bool FCharacterKitAnimProxy::Evaluate(FPoseContext& Output)
{
	if (Sequence)
	{
		const double Length = FMath::Max(Sequence->GetPlayLength(), 0.01);
		FAnimationPoseData PoseData(Output);
		Sequence->GetAnimationPose(PoseData, FAnimExtractContext(FMath::Fmod(Time, Length), false, {}, true));
	}
	else
	{
		Output.ResetToRefPose();
	}
	CharacterBodyProfile::ApplyToPose(Profile, Output.Pose, RefLegHeight);
	return true;
}
