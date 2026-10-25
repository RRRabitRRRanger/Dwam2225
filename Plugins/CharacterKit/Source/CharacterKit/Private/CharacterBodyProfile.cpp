#include "CharacterBodyProfile.h"
#include "BonePose.h"

namespace CharacterBodyProfile
{
	namespace
	{
		bool Is(const FString& Name, const TCHAR* Prefix)
		{
			return Name.StartsWith(Prefix);
		}
	}

	float BoneLengthFactor(const FCharacterBodyProfile& P, FName Bone)
	{
		const FString N = Bone.ToString();
		if (Is(N, TEXT("thigh")) || Is(N, TEXT("calf")))
		{
			return P.LegLength;
		}
		if (Is(N, TEXT("upperarm")) || Is(N, TEXT("lowerarm")))
		{
			return P.ArmLength;
		}
		if (Is(N, TEXT("spine_01")) || Is(N, TEXT("spine_02")) || Is(N, TEXT("spine_03")) || Is(N, TEXT("spine_04")))
		{
			return P.TorsoLength;
		}
		if (Is(N, TEXT("neck")))
		{
			return P.NeckLength;
		}
		if (Is(N, TEXT("clavicle")))
		{
			return P.ShoulderWidth;
		}
		if (Is(N, TEXT("head")))
		{
			return P.HeadSize;
		}
		return 1.f;
	}

	FVector BonePartFactors(const FCharacterBodyProfile& P, FName Bone)
	{
		const FString N = Bone.ToString();
		if (Is(N, TEXT("thigh")) || Is(N, TEXT("calf")))
		{
			return FVector(P.LegLength, P.LegThickness, P.LegThickness);
		}
		if (Is(N, TEXT("foot")) || Is(N, TEXT("ball")))
		{
			return FVector(1.f, P.LegThickness, P.LegThickness);
		}
		if (Is(N, TEXT("upperarm")) || Is(N, TEXT("lowerarm")))
		{
			return FVector(P.ArmLength, P.ArmThickness, P.ArmThickness);
		}
		if (Is(N, TEXT("hand")))
		{
			return FVector(1.f, P.ArmThickness, P.ArmThickness);
		}
		if (Is(N, TEXT("clavicle")))
		{
			return FVector(P.ShoulderWidth, P.ArmThickness, P.ArmThickness);
		}
		if (Is(N, TEXT("pelvis")))
		{
			return FVector(1.f, P.HipWidth, FMath::Sqrt(P.HipWidth * P.WaistSize));
		}
		if (Is(N, TEXT("spine_01")) || Is(N, TEXT("spine_02")))
		{
			return FVector(P.TorsoLength, P.WaistSize, P.WaistSize);
		}
		if (Is(N, TEXT("spine_03")))
		{
			const float Mid = 0.5f * (P.WaistSize + P.ChestSize);
			return FVector(P.TorsoLength, Mid, Mid);
		}
		if (Is(N, TEXT("spine_04")) || Is(N, TEXT("spine_05")))
		{
			return FVector(P.TorsoLength, P.ChestSize * FMath::Sqrt(P.ShoulderWidth), P.ChestSize);
		}
		if (Is(N, TEXT("neck")))
		{
			return FVector(P.NeckLength, 1.f, 1.f);
		}
		if (Is(N, TEXT("head")))
		{
			return FVector(P.HeadSize);
		}
		return FVector::OneVector;
	}

	FTransform ApplyToPartOffset(const FCharacterBodyProfile& P, FName Bone, const FTransform& Offset, bool bIgnoreLength)
	{
		// Convention des pièces : X latéral, Y profondeur, Z le long de l'os (celle de MakeProxyDescription).
		// L'os a son X le long de lui-même. On répartit chaque axe de la pièce entre « le long » et « en travers ».
		const FVector F = BonePartFactors(P, Bone);
		const float Along = bIgnoreLength ? 0.5f * (F.Y + F.Z) : F.X;
		const float Cross[3] = { F.Y, F.Z, F.Y };
		const FQuat R = Offset.GetRotation();
		FVector Scale = Offset.GetScale3D();
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const FVector AxisInBone = R.RotateVector(Axis == 0 ? FVector::XAxisVector : Axis == 1 ? FVector::YAxisVector : FVector::ZAxisVector);
			const float A = FMath::Abs(AxisInBone.X);
			Scale[Axis] *= A * Along + (1.f - A) * Cross[Axis];
		}
		const float CrossMean = 0.5f * (F.Y + F.Z);
		const FVector L = Offset.GetLocation();
		return FTransform(R, FVector(L.X * (bIgnoreLength ? 1.f : F.X), L.Y * CrossMean, L.Z * CrossMean), Scale);
	}

	void ApplyToPose(const FCharacterBodyProfile& P, FCompactPose& Pose, float RefLegHeight)
	{
		const FBoneContainer& Bones = Pose.GetBoneContainer();
		const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
		for (FCompactPoseBoneIndex Index : Pose.ForEachBoneIndex())
		{
			const int32 MeshIndex = Bones.MakeMeshPoseIndex(Index).GetInt();
			const int32 ParentMesh = Ref.GetParentIndex(MeshIndex);
			FTransform& T = Pose[Index];
			const FString Name = Ref.GetBoneName(MeshIndex).ToString();
			if (ParentMesh == INDEX_NONE)
			{
				T.SetScale3D(T.GetScale3D() * P.Height);
				continue;
			}
			// L'os parent s'allonge : la translation de cet enfant grandit d'autant
			float Factor = BoneLengthFactor(P, Ref.GetBoneName(ParentMesh));
			if (Name.StartsWith(TEXT("thigh")) && !Name.Contains(TEXT("twist")))
			{
				Factor *= P.HipWidth;
			}
			T.SetTranslation(T.GetTranslation() * Factor);
			if (Name == TEXT("pelvis"))
			{
				// Les pieds restent au sol : le bassin monte ou descend avec les jambes
				T.AddToTranslation(FVector(0.f, 0.f, RefLegHeight * (P.LegLength - 1.f)));
			}
		}
	}

	TArray<TPair<FString, FCharacterBodyProfile>> Presets()
	{
		TArray<TPair<FString, FCharacterBodyProfile>> Out;
		{
			FCharacterBodyProfile P;
			Out.Add({ TEXT("Standard"), P });
		}
		{
			FCharacterBodyProfile P;
			P.Height = 0.62f; P.LegLength = 0.85f; P.ArmLength = 0.9f; P.TorsoLength = 0.95f; P.NeckLength = 0.8f;
			P.HeadSize = 1.35f; P.ShoulderWidth = 0.8f; P.ChestSize = 0.9f; P.WaistSize = 1.f; P.HipWidth = 0.85f;
			P.ArmThickness = 0.9f; P.LegThickness = 0.95f;
			Out.Add({ TEXT("Enfant"), P });
		}
		{
			FCharacterBodyProfile P;
			P.LegLength = 1.08f; P.ArmLength = 1.05f; P.NeckLength = 1.15f; P.ShoulderWidth = 0.85f; P.ChestSize = 0.8f;
			P.WaistSize = 0.78f; P.HipWidth = 0.9f; P.ArmThickness = 0.7f; P.LegThickness = 0.72f;
			Out.Add({ TEXT("Mince"), P });
		}
		{
			FCharacterBodyProfile P;
			P.LegLength = 0.92f; P.NeckLength = 0.8f; P.ShoulderWidth = 1.05f; P.ChestSize = 1.4f; P.WaistSize = 1.75f;
			P.HipWidth = 1.35f; P.ArmThickness = 1.35f; P.LegThickness = 1.45f; P.HeadSize = 1.05f;
			Out.Add({ TEXT("Lourd"), P });
		}
		{
			FCharacterBodyProfile P;
			P.Height = 1.04f; P.NeckLength = 0.85f; P.ShoulderWidth = 1.3f; P.ChestSize = 1.35f; P.WaistSize = 1.0f;
			P.HipWidth = 1.f; P.ArmThickness = 1.45f; P.LegThickness = 1.3f; P.HeadSize = 0.95f;
			Out.Add({ TEXT("Muscle"), P });
		}
		{
			FCharacterBodyProfile P;
			P.Height = 1.25f; P.LegLength = 1.15f; P.ArmLength = 1.12f; P.TorsoLength = 1.05f; P.HeadSize = 0.9f;
			P.ShoulderWidth = 1.1f; P.ArmThickness = 0.95f; P.LegThickness = 0.95f;
			Out.Add({ TEXT("Grand"), P });
		}
		return Out;
	}
}
