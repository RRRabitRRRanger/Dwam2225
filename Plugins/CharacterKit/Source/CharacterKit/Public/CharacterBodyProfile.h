#pragma once

#include "CoreMinimal.h"
#include "CharacterPreset.h"

struct FCompactPose;

/**
 * Application du profil de corps (FCharacterBodyProfile) : au squelette (longueurs) et aux pièces (épaisseurs).
 * Les groupes sont reconnus par le nom des os du mannequin UE5 (thigh_l, spine_03…).
 */
namespace CharacterBodyProfile
{
	/** Facteur de longueur de l'os (s'applique à la translation de ses enfants). */
	CHARACTERKIT_API float BoneLengthFactor(const FCharacterBodyProfile& Profile, FName Bone);

	/** Facteurs d'une pièce posée sur cet os : le long de l'os (X de l'os), et en travers (latéral, profondeur). */
	CHARACTERKIT_API FVector BonePartFactors(const FCharacterBodyProfile& Profile, FName Bone);

	/** Décalage d'une pièce : position et échelle mises au profil (la pièce suit l'os allongé et s'épaissit). */
	CHARACTERKIT_API FTransform ApplyToPartOffset(const FCharacterBodyProfile& Profile, FName Bone, const FTransform& Offset, bool bIgnoreLength = false);

	/**
	 * Allonge les os d'une pose locale (espace parent) selon le profil, et relève le bassin pour que les pieds
	 * restent au sol quand les jambes changent de longueur. RefLegHeight : hauteur bassin → cheville en pose de référence.
	 */
	CHARACTERKIT_API void ApplyToPose(const FCharacterBodyProfile& Profile, FCompactPose& Pose, float RefLegHeight);

	/** Préréglages de proportions (planches de torses : enfant, mince, standard, lourd, musclé, grand). */
	CHARACTERKIT_API TArray<TPair<FString, FCharacterBodyProfile>> Presets();
}
