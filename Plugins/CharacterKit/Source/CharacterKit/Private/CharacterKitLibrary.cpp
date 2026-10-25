#include "CharacterKitLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "ReferenceSkeleton.h"

namespace CharacterKitProxy
{
	enum class EShape : uint8 { Box, Cylinder, Sphere };

	/** Une forme sur un os : vers quel enfant elle s'étire, et sa section (largeur latérale, profondeur), en cm. */
	struct FSpec
	{
		const TCHAR* Bone;
		const TCHAR* Child;
		EShape Shape;
		float Width;
		float Depth;
		bool bMirror;
	};

	// Tronc, cou, tête ; puis un côté (bMirror : décliné en _l et _r)
	const FSpec Specs[] =
	{
		{ TEXT("pelvis"),     TEXT("spine_01"),  EShape::Box,      30.f, 20.f, false },
		{ TEXT("spine_01"),   TEXT("spine_02"),  EShape::Box,      28.f, 18.f, false },
		{ TEXT("spine_02"),   TEXT("spine_03"),  EShape::Box,      29.f, 19.f, false },
		{ TEXT("spine_03"),   TEXT("spine_04"),  EShape::Box,      32.f, 20.f, false },
		{ TEXT("spine_04"),   TEXT("spine_05"),  EShape::Box,      34.f, 21.f, false },
		{ TEXT("spine_05"),   TEXT("neck_01"),   EShape::Box,      30.f, 18.f, false },
		{ TEXT("neck_01"),    TEXT("neck_02"),   EShape::Cylinder, 10.f, 10.f, false },
		{ TEXT("neck_02"),    TEXT("head"),      EShape::Cylinder, 10.f, 10.f, false },
		{ TEXT("head"),       nullptr,           EShape::Sphere,   17.f, 21.f, false },
		{ TEXT("clavicle"),   TEXT("upperarm"),  EShape::Cylinder,  8.f,  8.f, true },
		{ TEXT("upperarm"),   TEXT("lowerarm"),  EShape::Cylinder, 10.f, 10.f, true },
		{ TEXT("lowerarm"),   TEXT("hand"),      EShape::Cylinder,  8.f,  8.f, true },
		{ TEXT("hand"),       TEXT("middle_metacarpal|middle_01"), EShape::Box,       8.f,  3.f, true },
		{ TEXT("thigh"),      TEXT("calf"),      EShape::Cylinder, 15.f, 15.f, true },
		{ TEXT("calf"),       TEXT("foot"),      EShape::Cylinder, 11.f, 11.f, true },
		{ TEXT("foot"),       TEXT("ball"),      EShape::Box,       9.f,  7.f, true },
	};

	FName SideName(const TCHAR* Base, const TCHAR* Side)
	{
		return Side ? FName(FString::Printf(TEXT("%s_%s"), Base, Side)) : FName(Base);
	}

	/** Transform de chaque os dans l'espace du composant, en pose de référence. */
	TArray<FTransform> ComponentSpacePose(const FReferenceSkeleton& Ref)
	{
		const TArray<FTransform>& Local = Ref.GetRefBonePose();
		TArray<FTransform> CS;
		CS.SetNum(Local.Num());
		for (int32 i = 0; i < Local.Num(); ++i)
		{
			const int32 Parent = Ref.GetParentIndex(i);
			CS[i] = Parent == INDEX_NONE ? Local[i] : Local[i] * CS[Parent];
		}
		return CS;
	}
}

FCharacterDescription UCharacterKitLibrary::MakeProxyDescription(const USkeletalMesh* Mesh)
{
	using namespace CharacterKitProxy;

	FCharacterDescription Desc;
	if (!Mesh)
	{
		return Desc;
	}
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	const TArray<FTransform> CS = ComponentSpacePose(Ref);

	for (const FSpec& Spec : Specs)
	{
		static const TCHAR* Both[] = { TEXT("l"), TEXT("r") };
		static const TCHAR* None[] = { nullptr };
		const TArrayView<const TCHAR*> Sides = Spec.bMirror ? TArrayView<const TCHAR*>(Both) : TArrayView<const TCHAR*>(None);

		for (const TCHAR* Side : Sides)
		{
			const FName Bone = SideName(Spec.Bone, Side);
			const int32 BoneIndex = Ref.FindBoneIndex(Bone);
			if (BoneIndex == INDEX_NONE)
			{
				continue;
			}
			const FTransform& BoneCS = CS[BoneIndex];

			// Repère du personnage (le mannequin regarde vers +Y, haut = +Z) exprimé dans le repère de l'os
			const FVector Lateral = BoneCS.InverseTransformVectorNoScale(FVector::XAxisVector);
			const FVector Up = BoneCS.InverseTransformVectorNoScale(FVector::ZAxisVector);

			FVector Dir;
			float Length;
			if (Spec.Child)
			{
				// Plusieurs noms possibles, séparés par « | » (les squelettes varient)
				int32 ChildIndex = INDEX_NONE;
				TArray<FString> Candidates;
				FString(Spec.Child).ParseIntoArray(Candidates, TEXT("|"));
				for (const FString& Candidate : Candidates)
				{
					ChildIndex = Ref.FindBoneIndex(SideName(*Candidate, Side));
					if (ChildIndex != INDEX_NONE)
					{
						break;
					}
				}
				// Repli : l'enfant direct le plus éloigné
				if (ChildIndex == INDEX_NONE)
				{
					float Best = 0.f;
					for (int32 i = 0; i < Ref.GetRawBoneNum(); ++i)
					{
						const float D = FVector::Dist(CS[i].GetLocation(), BoneCS.GetLocation());
						if (Ref.GetParentIndex(i) == BoneIndex && D > Best)
						{
							Best = D;
							ChildIndex = i;
						}
					}
				}
				if (ChildIndex == INDEX_NONE)
				{
					continue;
				}
				const FVector ChildLocal = BoneCS.InverseTransformPositionNoScale(CS[ChildIndex].GetLocation());
				Length = ChildLocal.Size();
				Dir = ChildLocal / FMath::Max(Length, 0.01f);
			}
			else
			{
				Dir = Up;
				Length = 24.f;
			}

			FCharacterPart Part;
			Part.Slot = Bone;
			Part.Bone = Bone;

			// Le Z de la forme suit l'os, son X reste latéral : la largeur est toujours de gauche à droite
			const FRotator Rot = FRotationMatrix::MakeFromZX(Dir, Lateral).Rotator();
			FVector Center = Dir * Length * 0.5f;
			FVector Size(Spec.Width, Spec.Depth, Length);
			switch (Spec.Shape)
			{
			case EShape::Box:
				Part.Mesh = Cube;
				break;
			case EShape::Cylinder:
				Part.Mesh = Cylinder;
				Size.Z = Length + Spec.Width * 0.5f; // un peu plus long : les cylindres se chevauchent aux articulations
				break;
			case EShape::Sphere:
				Part.Mesh = Sphere;
				Center = Up * 9.f + BoneCS.InverseTransformVectorNoScale(FVector::YAxisVector) * 2.f;
				break;
			}
			Part.Offset = FTransform(Rot, Center, Size / 100.f);
			Desc.Parts.Add(Part);
		}
	}
	// Rotules aux articulations, comme sur les figurines : une sphère à l'origine de l'os, qui ne s'allonge pas avec lui
	struct FJoint { const TCHAR* Bone; float Diameter; };
	static const FJoint Joints[] =
	{
		{ TEXT("upperarm"), 11.f }, { TEXT("lowerarm"), 9.f }, { TEXT("hand"), 7.f },
		{ TEXT("thigh"), 16.f }, { TEXT("calf"), 12.f }, { TEXT("foot"), 9.f },
	};
	for (const FJoint& Joint : Joints)
	{
		for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
		{
			const FName Bone = SideName(Joint.Bone, Side);
			if (Ref.FindBoneIndex(Bone) == INDEX_NONE)
			{
				continue;
			}
			FCharacterPart Part;
			Part.Slot = FName(FString::Printf(TEXT("Joint_%s"), *Bone.ToString()));
			Part.Bone = Bone;
			Part.Mesh = Sphere;
			Part.bIgnoreLength = true;
			Part.Offset = FTransform(FQuat::Identity, FVector::ZeroVector, FVector(Joint.Diameter / 100.f));
			Desc.Parts.Add(Part);
		}
	}
	return Desc;
}
