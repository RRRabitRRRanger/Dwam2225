#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LOATypes.generated.h"

struct FGraphKitGraph;
struct FGraphKitNode;

/**
 * LOA : Level of Abstraction (niveau d'abstraction).
 * 0 = le plus détaillé ; plus le niveau monte, plus l'objet est abstrait (jusqu'à « une maison Pokémon »).
 * La partie entière fait la structure (ce qui existe), la partie décimale fait le dépliage (ce qui pousse).
 */

/** Ce que devient un élément quand le LOA dépasse son niveau. */
UENUM(BlueprintType)
enum class ELOACollapse : uint8
{
	/** Il s'efface (une fenêtre, une rambarde, une terrasse). */
	Vanish UMETA(DisplayName = "S'efface"),
	/** Il se fond dans son parent (une grange qui devient un seul volume avec la maison). */
	Absorb UMETA(DisplayName = "Absorbé par le parent")
};

/** Réglage LOA d'un élément (à mettre dans la charge utile d'un nœud). */
USTRUCT(BlueprintType)
struct LOA_API FLOAElement
{
	GENERATED_BODY()

	/** Visible jusqu'à ce niveau (inclus). Entre ce niveau et le suivant, il se replie progressivement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOA", meta = (ClampMin = "0"))
	int32 VisibleUpTo = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOA")
	ELOACollapse Collapse = ELOACollapse::Vanish;

	/** Présence propre (0 à 1) pour un niveau donné : 1 jusqu'à VisibleUpTo, 0 à partir de VisibleUpTo + 1. */
	float Presence(float Level) const { return FMath::Clamp(VisibleUpTo + 1.f - Level, 0.f, 1.f); }
};

namespace LOA
{
	/**
	 * Présence (0 à 1) de chaque nœud atteignable depuis Root (arêtes sortantes), pour un niveau donné.
	 * En cascade : un enfant n'est jamais plus présent que son parent. La racine est toujours présente.
	 * GetElement renvoie le réglage LOA d'un nœud, ou nullptr (toujours présent).
	 */
	LOA_API void ComputePresence(const FGraphKitGraph& Graph, FName Root, float Level,
		TFunctionRef<const FLOAElement*(const FGraphKitNode&)> GetElement, TMap<FName, float>& OutPresence);
}

UCLASS()
class LOA_API ULOALibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Présence (0 à 1) d'un élément à un niveau d'abstraction donné (sans tenir compte de son parent). */
	UFUNCTION(BlueprintPure, Category = "LOA")
	static float GetPresence(const FLOAElement& Element, float Level) { return Element.Presence(Level); }
};
