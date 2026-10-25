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

	/** La pièce ne s'allonge pas avec l'os (rotules, ornements) : seule l'épaisseur du profil s'applique. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Part")
	bool bIgnoreLength = false;
};

/**
 * Les proportions du corps, par groupes d'os (1 = le mannequin d'origine).
 * Longueurs : appliquées au squelette (les os s'allongent, les pièces suivent).
 * Épaisseurs et largeurs : appliquées aux pièces.
 */
USTRUCT(BlueprintType)
struct CHARACTERKIT_API FCharacterBodyProfile
{
	GENERATED_BODY()

	/** Échelle globale (taille d'un enfant, d'un géant). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Length", meta = (ClampMin = "0.2", ClampMax = "3"))
	float Height = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Length", meta = (ClampMin = "0.3", ClampMax = "2.5"))
	float LegLength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Length", meta = (ClampMin = "0.3", ClampMax = "2.5"))
	float ArmLength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Length", meta = (ClampMin = "0.3", ClampMax = "2.5"))
	float TorsoLength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Length", meta = (ClampMin = "0.3", ClampMax = "3"))
	float NeckLength = 1.f;

	/** Taille de la tête (la tête d'un enfant est grande par rapport au corps). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Width", meta = (ClampMin = "0.3", ClampMax = "3"))
	float HeadSize = 1.f;

	/** Écart des épaules (longueur des clavicules) et largeur du haut du buste. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Width", meta = (ClampMin = "0.3", ClampMax = "3"))
	float ShoulderWidth = 1.f;

	/** Volume de la cage thoracique. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Width", meta = (ClampMin = "0.3", ClampMax = "3"))
	float ChestSize = 1.f;

	/** Ventre et taille. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Width", meta = (ClampMin = "0.3", ClampMax = "3"))
	float WaistSize = 1.f;

	/** Bassin : largeur et écart des hanches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Width", meta = (ClampMin = "0.3", ClampMax = "3"))
	float HipWidth = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Width", meta = (ClampMin = "0.3", ClampMax = "3"))
	float ArmThickness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Width", meta = (ClampMin = "0.3", ClampMax = "3"))
	float LegThickness = 1.f;
};

/**
 * La description d'un personnage : de simples données, qu'on copie, modifie et sauvegarde
 * (outil d'éditeur, écran du joueur, sauvegarde, PNJ générés).
 * Plus tard : ancres, valeurs des curseurs.
 */
USTRUCT(BlueprintType)
struct CHARACTERKIT_API FCharacterDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character", meta = (TitleProperty = "Slot"))
	TArray<FCharacterPart> Parts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FCharacterBodyProfile Profile;
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
