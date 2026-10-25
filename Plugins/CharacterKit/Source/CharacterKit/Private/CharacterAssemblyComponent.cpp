#include "CharacterAssemblyComponent.h"
#include "CharacterBodyProfile.h"
#include "CharacterKitAnimInstance.h"
#include "SDFEyeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterKit, Log, All);

void UCharacterAssemblyComponent::OnRegister()
{
	Super::OnRegister();
	Assemble();
}

void UCharacterAssemblyComponent::OnUnregister()
{
	ClearParts();
	Super::OnUnregister();
}

#if WITH_EDITOR
void UCharacterAssemblyComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	Assemble();
}
#endif

USkeletalMeshComponent* UCharacterAssemblyComponent::FindSkeleton() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	TArray<USkeletalMeshComponent*> Skeletons;
	Owner->GetComponents(Skeletons);
	for (USkeletalMeshComponent* Skeleton : Skeletons)
	{
		if (SkeletonComponentName.IsNone() || Skeleton->GetFName() == SkeletonComponentName)
		{
			return Skeleton;
		}
	}
	return nullptr;
}

void UCharacterAssemblyComponent::ClearParts()
{
	for (UStaticMeshComponent* Part : PartComponents)
	{
		if (IsValid(Part))
		{
			Part->DestroyComponent();
		}
	}
	PartComponents.Reset();
	PartSlots.Reset();
	for (USceneComponent* Anchor : AnchorComponents)
	{
		if (IsValid(Anchor))
		{
			Anchor->DestroyComponent();
		}
	}
	AnchorComponents.Reset();
	AnchorNames.Reset();
}

void UCharacterAssemblyComponent::Assemble()
{
	ClearParts();

	AActor* Owner = GetOwner();
	USkeletalMeshComponent* Skeleton = FindSkeleton();
	const FCharacterDescription* Source = GetSource();
	if (!Owner || !Skeleton || !Source)
	{
		return;
	}
	Skeleton->SetVisibility(!bHideSkeletonMesh);

	// Profil de corps : longueurs dans le squelette (si l'anim instance le sait), épaisseurs sur les pièces
	if (UCharacterKitAnimInstance* Anim = Cast<UCharacterKitAnimInstance>(Skeleton->GetAnimInstance()))
	{
		Anim->SetProfile(Source->Profile);
	}

	for (const FCharacterPart& Part : Source->Parts)
	{
		if (!Part.Mesh)
		{
			continue;
		}
		if (Skeleton->GetBoneIndex(Part.Bone) == INDEX_NONE)
		{
			UE_LOG(LogCharacterKit, Warning, TEXT("%s : os '%s' introuvable pour la pièce '%s'"),
				*Owner->GetName(), *Part.Bone.ToString(), *Part.Slot.ToString());
			continue;
		}

		// Pièce transitoire : recréée à chaque assemblage, jamais sauvegardée dans le niveau ni le Blueprint
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
		Comp->CreationMethod = EComponentCreationMethod::Instance;
		Comp->SetStaticMesh(Part.Mesh);
		if (Part.Material)
		{
			for (int32 i = 0; i < Comp->GetNumMaterials(); ++i)
			{
				Comp->SetMaterial(i, Part.Material);
			}
		}
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetupAttachment(Skeleton, Part.Bone);
		Comp->SetRelativeTransform(CharacterBodyProfile::ApplyToPartOffset(Source->Profile, Part.Bone, Part.Offset, Part.bIgnoreLength));
		Comp->RegisterComponent();
		// Largeur du fondu des jointures, à l'échelle du corps (Custom Primitive Data 0, lue par le matériau de corps)
		Comp->SetCustomPrimitiveDataFloat(0, Source->Profile.JointBlendDistance * Source->Profile.Height * Skeleton->GetComponentScale().X);

		PartComponents.Add(Comp);
		PartSlots.Add(Part.Slot);
	}

	// Ancres : un component par ancre, attaché à son os (les yeux suivent la tête, l'épaule, n'importe quel os)
	for (const FCharacterAnchor& Anchor : Source->Anchors)
	{
		if (Skeleton->GetBoneIndex(Anchor.Bone) == INDEX_NONE)
		{
			UE_LOG(LogCharacterKit, Warning, TEXT("%s : os '%s' introuvable pour l'ancre '%s'"),
				*Owner->GetName(), *Anchor.Bone.ToString(), *Anchor.Name.ToString());
			continue;
		}
		const EObjectFlags Flags = RF_Transient | RF_TextExportTransient | RF_DuplicateTransient;
		USceneComponent* Comp = nullptr;
		if (Anchor.Kind == ECharacterAnchorKind::Eye)
		{
			USDFEyeComponent* Eye = NewObject<USDFEyeComponent>(Owner, NAME_None, Flags);
			Eye->Shape = Anchor.EyeShape;
			Eye->EyeMaterial = EyeMaterial;
			Eye->GlobeRadius = Anchor.GlobeRadius * CharacterBodyProfile::BonePartFactors(Source->Profile, Anchor.Bone).Y;
			Eye->bMirror = Anchor.bMirror;
			Comp = Eye;
		}
		else
		{
			Comp = NewObject<USceneComponent>(Owner, NAME_None, Flags);
		}
		Comp->CreationMethod = EComponentCreationMethod::Instance;
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetupAttachment(Skeleton, Anchor.Bone);
		const FTransform Placed = CharacterBodyProfile::ApplyToPartOffset(Source->Profile, Anchor.Bone, Anchor.Offset);
		Comp->SetRelativeLocationAndRotation(Placed.GetLocation(), Placed.GetRotation());
		Comp->RegisterComponent();
		AnchorComponents.Add(Comp);
		AnchorNames.Add(Anchor.Name);
	}
}

USceneComponent* UCharacterAssemblyComponent::GetAnchorComponent(FName Anchor) const
{
	const int32 Index = AnchorNames.IndexOfByKey(Anchor);
	return AnchorComponents.IsValidIndex(Index) ? AnchorComponents[Index].Get() : nullptr;
}

void UCharacterAssemblyComponent::AddAnchor(const FCharacterAnchor& Anchor)
{
	TakeOwnDescription();
	Description.Anchors.Add(Anchor);
	Assemble();
}

const FCharacterDescription* UCharacterAssemblyComponent::GetSource() const
{
	if (bUsePreset)
	{
		return Preset ? &Preset->Description : nullptr;
	}
	return &Description;
}

FCharacterDescription UCharacterAssemblyComponent::GetCurrentDescription() const
{
	const FCharacterDescription* Source = GetSource();
	return Source ? *Source : FCharacterDescription();
}

void UCharacterAssemblyComponent::TakeOwnDescription()
{
	if (bUsePreset)
	{
		Description = GetCurrentDescription();
		bUsePreset = false;
	}
}

void UCharacterAssemblyComponent::SetDescription(const FCharacterDescription& NewDescription)
{
	Description = NewDescription;
	bUsePreset = false;
	Assemble();
}

bool UCharacterAssemblyComponent::SetPartMesh(FName Slot, UStaticMesh* NewMesh)
{
	TakeOwnDescription();
	FCharacterPart* Part = Description.Parts.FindByPredicate([Slot](const FCharacterPart& P) { return P.Slot == Slot; });
	if (!Part)
	{
		return false;
	}
	Part->Mesh = NewMesh;

	// La description reste la référence ; on met à jour la pièce visible sans tout réassembler
	if (UStaticMeshComponent* Comp = GetPartComponent(Slot))
	{
		Comp->SetStaticMesh(NewMesh);
	}
	else
	{
		Assemble();
	}
	return true;
}

UStaticMeshComponent* UCharacterAssemblyComponent::GetPartComponent(FName Slot) const
{
	const int32 Index = PartSlots.IndexOfByKey(Slot);
	return PartComponents.IsValidIndex(Index) ? PartComponents[Index].Get() : nullptr;
}
