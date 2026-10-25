#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterPreset.h"
#include "CharacterKitLibrary.generated.h"

class USkeletalMesh;

UCLASS()
class CHARACTERKIT_API UCharacterKitLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Construit un mannequin de formes simples à partir du squelette (pose de référence) :
	 * chaque os majeur reçoit une forme orientée vers son enfant, à la bonne longueur (cubes pour le tronc,
	 * les mains et les pieds, cylindres pour les membres et le cou, sphère pour la tête). Symétrique par construction.
	 * Sert de point de départ (pièces de test, outil d'éditeur). Noms d'os du mannequin UE5.
	 */
	UFUNCTION(BlueprintCallable, Category = "CharacterKit")
	static FCharacterDescription MakeProxyDescription(const USkeletalMesh* Mesh);

	/**
	 * Une ancre posée dans le repère du personnage (pose de référence : le mannequin regarde vers +Y, haut = +Z),
	 * convertie dans le repère de l'os. Pour un œil : Forward = où il regarde, Up = le haut de l'œil ;
	 * le côté du nez (miroir) est déduit de la position par rapport au plan X = 0.
	 */
	UFUNCTION(BlueprintCallable, Category = "CharacterKit")
	static FCharacterAnchor MakeAnchor(const USkeletalMesh* Mesh, FName Name, FName Bone, FVector Location, FVector Forward, FVector Up,
		ECharacterAnchorKind Kind = ECharacterAnchorKind::Eye);
};
