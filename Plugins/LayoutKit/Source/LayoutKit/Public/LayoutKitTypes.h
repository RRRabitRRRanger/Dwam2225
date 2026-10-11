#pragma once

#include "CoreMinimal.h"
#include "LOATypes.h"
#include "LayoutKitTypes.generated.h"

/** Comme en UI : une taille fixe (taille humaine, ne bouge pas) ou étirable (suit l'échelle). */
UENUM(BlueprintType)
enum class ELayoutKitSizeRule : uint8
{
	Fixed   UMETA(DisplayName = "Fixe"),
	Stretch UMETA(DisplayName = "Étirable")
};

/** Une dimension : une valeur (cm) et sa règle. */
USTRUCT(BlueprintType)
struct LAYOUTKIT_API FLayoutKitLength
{
	GENERATED_BODY()

	FLayoutKitLength() = default;
	FLayoutKitLength(float InValue, ELayoutKitSizeRule InRule) : Value(InValue), Rule(InRule) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "1", Units = "Centimeters"))
	float Value = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	ELayoutKitSizeRule Rule = ELayoutKitSizeRule::Stretch;

	/** Taille finale selon l'échelle demandée sur cet axe. */
	float Resolve(float Scale) const { return Rule == ELayoutKitSizeRule::Stretch ? Value * Scale : Value; }
};

/**
 * Une boîte (charge utile d'un nœud GraphKit).
 * Repère local : X = profondeur, l'avant est en +X ; Y = largeur ; Z = hauteur, posée au sol.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Layout Kit Box"))
struct LAYOUTKIT_API FLayoutKitBox
{
	GENERATED_BODY()

	/** Largeur (gauche-droite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FLayoutKitLength Width = FLayoutKitLength(1000.f, ELayoutKitSizeRule::Stretch);

	/** Profondeur (avant-arrière). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FLayoutKitLength Depth = FLayoutKitLength(800.f, ELayoutKitSizeRule::Stretch);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FLayoutKitLength Height = FLayoutKitLength(600.f, ELayoutKitSizeRule::Stretch);

	/** Rayons des coins (cm), comme le border-radius du CSS. 0 = coin vif. Réduits automatiquement s'ils ne tiennent pas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Corners", meta = (ClampMin = "0", Units = "Centimeters"))
	float FrontLeftRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Corners", meta = (ClampMin = "0", Units = "Centimeters"))
	float FrontRightRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Corners", meta = (ClampMin = "0", Units = "Centimeters"))
	float BackRightRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Corners", meta = (ClampMin = "0", Units = "Centimeters"))
	float BackLeftRadius = 0.f;

	/** Nombre de segments par arrondi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Corners", meta = (ClampMin = "1", ClampMax = "64"))
	int32 CornerSegments = 8;

	/** Niveau d'abstraction : jusqu'où la boîte existe, et ce qu'elle devient au-delà (s'efface ou se fond dans son parent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|LOA")
	FLOAElement LOA;
};

UENUM(BlueprintType)
enum class ELayoutKitSide : uint8
{
	Front UMETA(DisplayName = "Avant"),
	Back  UMETA(DisplayName = "Arrière"),
	Left  UMETA(DisplayName = "Gauche"),
	Right UMETA(DisplayName = "Droite")
};

/**
 * Une attache (charge utile d'une arête parent -> enfant).
 * L'enfant est collé au côté du parent, tourné vers l'extérieur : son avant regarde dehors.
 * Il dépasse de (sa profondeur - recouvrement).
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Layout Kit Attach"))
struct LAYOUTKIT_API FLayoutKitAttach
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	ELayoutKitSide Side = ELayoutKitSide::Front;

	/** Position le long du côté : 0 = début, 0,5 = centre, 1 = fin (l'enfant reste dans le côté). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0", ClampMax = "1"))
	float Alignment = 0.5f;

	/** De combien l'enfant entre dans le parent, pour que les volumes se touchent (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0", Units = "Centimeters"))
	float Overlap = 20.f;
};
