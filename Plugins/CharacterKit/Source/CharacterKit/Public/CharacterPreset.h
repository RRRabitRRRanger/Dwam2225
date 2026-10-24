#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterPreset.generated.h"

class UStaticMesh;
class UMaterialInterface;

/** Une pièce rigide du corps, attachée à un os du squelette. */
USTRUCT(BlueprintType)
struct CHARACTERKIT_API FCharacterPart
{
	GENERATED_BODY()

	/** Emplacement (tête, avant-bras gauche…) : sert à remplacer une pièce par une autre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Part")
	FName Slot;

	/** Os du squelette qui porte la pièce (ex. lowerarm_l). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Part")
	FName Bone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Part")
	TObjectPtr<UStaticMesh> Mesh;

	/** Décalage, rotation et échelle de la pièce dans le repère de l'os. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Part")
	FTransform Offset;

	/** Matériau qui remplace celui du mesh (vide = celui du mesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Part")
	TObjectPtr<UMaterialInterface> Material;
};

/**
 * La description d'un personnage : de simples données, qu'on copie, modifie et sauvegarde
 * (outil d'éditeur, écran du joueur, sauvegarde, PNJ générés).
 * Plus tard : profil de déformation, ancres, valeurs des curseurs.
 */
USTRUCT(BlueprintType)
struct CHARACTERKIT_API FCharacterDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character", meta = (TitleProperty = "Slot"))
	TArray<FCharacterPart> Parts;
};

/** Un préréglage : un asset (en lecture seule en jeu) qui contient une description de départ. */
UCLASS(BlueprintType)
class CHARACTERKIT_API UCharacterPreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character", meta = (ShowOnlyInnerProperties))
	FCharacterDescription Description;
};
