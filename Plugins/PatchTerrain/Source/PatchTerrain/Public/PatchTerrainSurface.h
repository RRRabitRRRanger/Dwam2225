#pragma once

#include "CoreMinimal.h"
#include "PatchTerrainTypes.h"
#include "PatchTerrainExpression.h"

/**
 * Description d'un tampon, déjà exprimée dans l'espace local du terrain.
 * Structure simple (sans UObject) : la surface ne dépend pas des acteurs.
 */
struct FPatchTerrainStampDesc
{
	EPatchTerrainStampShape Shape = EPatchTerrainStampShape::Hill;
	EPatchTerrainStampBlend Blend = EPatchTerrainStampBlend::Add;

	/** Centre de la base du tampon (espace local du terrain). */
	FVector Center = FVector::ZeroVector;
	/** Rotation autour de Z, en degrés. */
	double Yaw = 0.0;
	/** Demi-emprise (cm) le long des axes X / Y locaux du tampon. */
	FVector2D HalfSize = FVector2D(500.0, 500.0);
	/** Hauteur (cm) correspondant à une valeur de forme de 1. */
	double Height = 500.0;

	double Squareness = 0.0;
	double EdgeSoftness = 0.25;
	double Frequency = 3.0;
	double Shaping = 0.5;
	FString Expression;

	/** Rampe : relie la hauteur du terrain sous ses deux extrémités (ignore la hauteur et le mode de mélange). */
	bool bAutoFitEnds = false;

	int32 StepCount = 0;
	double StepSoftness = 0.5;

	EPatchTerrainStampNoise NoiseType = EPatchTerrainStampNoise::None;
	double NoiseAmount = 0.2;
	double NoiseScale = 1000.0;
	int32 NoiseOctaves = 4;
	int32 Seed = 0;
};

/**
 * Description d'une lattice (déformation de forme libre, FFD de Bézier) dans l'espace local du terrain.
 * Repère de la boîte : X, Y dans [BoxMin, BoxMax], Z de BoxMin.Z à BoxMax.Z (avant l'échelle de l'acteur).
 */
struct FPatchTerrainLatticeDesc
{
	/** Repère de la boîte -> espace local du terrain (avec l'échelle). */
	FTransform BoxToTerrain = FTransform::Identity;
	FVector BoxMin = FVector(-500.0, -500.0, 0.0);
	FVector BoxMax = FVector(500.0, 500.0, 500.0);
	/** Nombre de points de contrôle par axe (2 à 5). */
	FIntVector Resolution = FIntVector(3, 3, 2);
	/** Points de contrôle dans le repère de la boîte ; index = (K * Ry + J) * Rx + I. */
	TArray<FVector> ControlPoints;
	/** Fondu de la déformation près des faces, en fraction de la boîte (0 = aucun). */
	double Softness = 0.15;
};

/**
 * Évaluation mathématique du terrain, sans dépendance aux acteurs ni au rendu.
 *
 * Le canevas est une grille rectilinéaire de NX × NY nœuds :
 *  - KnotsX / KnotsY : position de chaque colonne / ligne de nœuds dans l'espace paramétrique (cm),
 *    croissante ; les écarts entre nœuds sont libres (cases de tailles différentes) ;
 *  - Points : un point de contrôle 3D par nœud (espace local), rangé ligne par ligne (index = Y * NX + X).
 *
 * La surface de base est un Catmull-Rom bicubique non uniforme : elle passe par les points de contrôle
 * et reste continue en pente (C1) d'une case à l'autre, quelles que soient leurs tailles.
 * La pile de modificateurs s'applique ensuite sur Z.
 *
 * Toutes les méthodes d'évaluation sont const et sans état : utilisables en parallèle.
 */
class PATCHTERRAIN_API FPatchTerrainSurface
{
public:
	/**
	 * Prépare la surface. Retourne false si les données sont incohérentes.
	 * OutModifierStatus (optionnel) reçoit un message par modificateur d'entrée (vide = OK, sinon erreur d'expression).
	 */
	bool Setup(
		TArray<double> InKnotsX,
		TArray<double> InKnotsY,
		TArray<FVector> InPoints,
		const TArray<FPatchTerrainModifier>& InModifiers,
		int32 InSeed,
		TArray<FString>* OutModifierStatus = nullptr);

	/** Définit les lattices, appliquées en dernier sur la position 3D (permettent les surplombs). */
	void SetLattices(const TArray<FPatchTerrainLatticeDesc>& InLattices);

	/**
	 * Définit les tampons, appliqués après les modificateurs, dans l'ordre du tableau.
	 * OutStatus (optionnel) reçoit un message par tampon (vide = OK, sinon erreur d'expression).
	 */
	void SetStamps(const TArray<FPatchTerrainStampDesc>& InStamps, int32 InSeed, TArray<FString>* OutStatus = nullptr);

	bool IsValid() const { return bValid; }

	int32 GetNumNodesX() const { return KnotsX.Num(); }
	int32 GetNumNodesY() const { return KnotsY.Num(); }
	const TArray<double>& GetKnotsX() const { return KnotsX; }
	const TArray<double>& GetKnotsY() const { return KnotsY; }

	/** Position finale (base + modificateurs) au point paramétrique (X, Y). Les coordonnées sont bornées au canevas. */
	FVector Evaluate(double X, double Y) const;

	/** Position et normale (différences finies centrées de pas Epsilon, cohérentes d'un chunk à l'autre). */
	void EvaluateWithNormal(double X, double Y, double Epsilon, FVector& OutPosition, FVector& OutNormal) const;

	/** Surface de base seule, sans modificateurs. */
	FVector EvaluateBase(double X, double Y) const;

private:
	/** Modificateur avec ses valeurs dérivées précalculées (zone, direction, décalage de bruit). */
	struct FPreparedModifier
	{
		FPatchTerrainModifier Params;
		FVector2D NoiseOffset = FVector2D::ZeroVector;
		FVector2D Dir = FVector2D(1.0, 0.0);
		FVector2D RectMin = FVector2D::ZeroVector;
		FVector2D RectMax = FVector2D::ZeroVector;
		TSharedPtr<FPatchTerrainExpression> Expression;
	};

	double ApplyModifiers(double Z, double X, double Y) const;

	struct FPreparedStamp
	{
		FPatchTerrainStampDesc Desc;
		double CosYaw = 1.0;
		double SinYaw = 0.0;
		FVector2D NoiseOffset = FVector2D::ZeroVector;
		/** Raccord auto (rampe) : hauteur du terrain sous le début et la fin. */
		double RampZ0 = 0.0;
		double RampZ1 = 0.0;
		TSharedPtr<FPatchTerrainExpression> Expression;
	};

	/** Applique les tampons à la hauteur Z, au point réel (X, Y) de la surface. */
	double ApplyStamps(double Z, double X, double Y, int32 NumStamps = INDEX_NONE) const;

	/** Applique les lattices à une position (espace local du terrain). */
	FVector ApplyLattices(const FVector& P) const;

	/** Hauteur au point (X, Y) avec seulement les NumStamps premiers tampons (sert au raccord auto des rampes). */
	double HeightBeforeStamp(double X, double Y, int32 NumStamps) const;
	static double StampShapeValue(const FPreparedStamp& Stamp, double S, double T, double R, double X, double Y, double Z);
	static double ModifierWeight(const FPreparedModifier& Mod, double X, double Y);

	TArray<double> KnotsX;
	TArray<double> KnotsY;
	TArray<FVector> Points;
	TArray<FPreparedModifier> Modifiers;
	TArray<FPreparedStamp> Stamps;
	TArray<FPatchTerrainLatticeDesc> Lattices;
	bool bValid = false;
};
