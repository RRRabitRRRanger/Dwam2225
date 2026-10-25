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

UENUM(BlueprintType)
enum class ECharacterAnchorKind : uint8
{
	/** Un simple point nommé (attaches de vêtements, armes plantées…). */
	Point,
	/** Un œil SDFEyes : regard au repos selon le X local, haut selon le Z local. */
	Eye
};

/** Un point nommé posé sur un os : un œil, plus tard une bouche, une attache. Autant qu'on veut, où on veut. */
USTRUCT(BlueprintType)
struct CHARACTERKIT_API FCharacterAnchor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor")
	FName Bone;

	/** Position et orientation dans le repère de l'os (l'échelle est ignorée pour un œil : voir GlobeRadius). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor")
	FTransform Offset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor")
	ECharacterAnchorKind Kind = ECharacterAnchorKind::Point;

	/** Œil : forme de paupière (vide = défauts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor|Eye", meta = (EditCondition = "Kind == ECharacterAnchorKind::Eye"))
	TObjectPtr<class USDFEyeShape> EyeShape;

	/** Œil : rayon du globe en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor|Eye", meta = (EditCondition = "Kind == ECharacterAnchorKind::Eye"))
	float GlobeRadius = 1.25f;

	/** Œil : nez de l'autre côté. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anchor|Eye", meta = (EditCondition = "Kind == ECharacterAnchorKind::Eye"))
	bool bMirror = false;
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

	/**
	 * Largeur du fondu des jointures (transfert de normales), en cm, pour un corps de taille 1.
	 * Multipliée par la taille du personnage (Height × échelle de l'acteur) et envoyée à chaque pièce
	 * en Custom Primitive Data, index 0 (essais du 2026-10-25 : le fondu doit suivre la taille du corps).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body|Joints", meta = (ClampMin = "1", ClampMax = "60"))
	float JointBlendDistance = 10.f;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character", meta = (TitleProperty = "Name"))
	TArray<FCharacterAnchor> Anchors;
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
