#pragma once

#include "CoreMinimal.h"

struct FGraphKitGraph;

namespace UE::Geometry { class FDynamicMesh3; }

/** Une boîte une fois placée (repère racine, tailles finales en cm). */
struct LAYOUTKIT_API FLayoutKitPlacedBox
{
	FName Name;
	/** Centre de l'empreinte dans le repère de la racine. */
	FVector2D Center = FVector2D::ZeroVector;
	/** Orientation par quarts de tour : l'avant local (+X) tourné de Quarter × 90° (X vers Y). */
	int32 Quarter = 0;
	float Width = 0.f;
	float Depth = 0.f;
	float Height = 0.f;
	/** Rayons après réduction : avant-gauche, avant-droite, arrière-droite, arrière-gauche. */
	float Radii[4] = { 0.f, 0.f, 0.f, 0.f };
	int32 Segments = 8;
};

/**
 * Calculs purs de LayoutKit (sans acteur, testables).
 */
namespace LayoutKit
{
	/** Tourne un vecteur 2D de Quarter × 90° (X vers Y). */
	LAYOUTKIT_API FVector2D RotateQuarter(const FVector2D& V, int32 Quarter);

	/** Réduit les rayons pour qu'ils tiennent : au plus la moitié du petit côté, et deux rayons d'un même côté pas plus longs que lui. */
	LAYOUTKIT_API void ClampRadii(float Width, float Depth, float InOutRadii[4]);

	/**
	 * Place toutes les boîtes du graphe, en partant de la racine et en suivant les arêtes (parent -> enfant).
	 * Scale : l'échelle demandée sur les axes de la racine (les tailles étirables la suivent, les fixes non).
	 * LOALevel : le niveau d'abstraction (0 = le plus détaillé). Une boîte qui s'efface rapetisse depuis son point
	 * d'attache ; une boîte absorbée prend la section de son parent (largeur du côté, hauteur, coins vifs).
	 * Les nœuds sans boîte (charge utile d'un autre type) sont ignorés et signalés dans Problems.
	 */
	LAYOUTKIT_API void SolveLayout(const FGraphKitGraph& Graph, FName Root, const FVector& Scale,
		TArray<FLayoutKitPlacedBox>& OutBoxes, TArray<FString>& OutProblems, float LOALevel = 0.f);

	/** L'exemple de départ : corps, entrée (toujours), terrasse (jusqu'au LOA 0, s'efface), grange à l'arrière (jusqu'au LOA 1, absorbée). */
	LAYOUTKIT_API void MakeExampleHouse(FGraphKitGraph& OutGraph, FName& OutRoot);

	/** Contour de l'empreinte (repère racine), dans le sens trigonométrique ; Sharp = coin vif à ce point. */
	LAYOUTKIT_API void BuildFootprint(const FLayoutKitPlacedBox& Box, TArray<FVector2D>& OutPoints, TArray<bool>& OutSharp);

	/** Ajoute un prisme fermé (murs, sol, toit) : arrondis lissés, coins vifs nets, UV en mètres. Le mesh doit avoir ses attributs activés. */
	LAYOUTKIT_API void AppendPrism(UE::Geometry::FDynamicMesh3& Mesh, const TArray<FVector2D>& Points, const TArray<bool>& Sharp, double Z0, double Z1);
}
