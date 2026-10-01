#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PatchTerrainTypes.h"
#include "PatchTerrainSurface.h"
#include "PatchTerrainStamp.generated.h"

class APatchTerrain;
class UBoxComponent;

/**
 * Tampon de terrain : une forme qu'on pose, déplace, tourne et étire avec les gizmos standard.
 *
 *  - Emprise : 10 m × 10 m à l'échelle 1 (scale X / Y pour l'étirer).
 *  - Hauteur : 5 m à l'échelle 1 (scale Z). La boîte filaire montre l'emprise et la hauteur.
 *  - Seule la rotation autour de Z (lacet) est prise en compte.
 *
 * Le terrain relit les tampons à chaque reconstruction, en éditeur comme en jeu.
 */
UCLASS(ClassGroup = Terrain, meta = (DisplayName = "Patch Terrain Stamp"))
class PATCHTERRAIN_API APatchTerrainStamp : public AActor
{
	GENERATED_BODY()

public:
	APatchTerrainStamp();

	/** Demi-emprise (cm) à l'échelle 1. */
	static constexpr double BaseHalfSize = 500.0;
	/** Hauteur (cm) à l'échelle 1. */
	static constexpr double BaseHeight = 500.0;

	// ---------------------------------------------------------------- Tampon

	/** Forme du tampon : se change à tout moment sans perdre le placement ni les réglages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon")
	EPatchTerrainStampShape Shape = EPatchTerrainStampShape::Hill;

	/** Combinaison avec le terrain en dessous. Max / Min / Aplatir utilisent l'altitude du tampon comme base. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon")
	EPatchTerrainStampBlend Blend = EPatchTerrainStampBlend::Add;

	/** Contour : 0 = rond (ellipse), 1 = carré (aux coins arrondis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon", meta = (ClampMin = "0", ClampMax = "1"))
	float Squareness = 0.f;

	/** Largeur des bords doux, en fraction de l'emprise (0 = bord net, 1 = fondu jusqu'au centre). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon", meta = (ClampMin = "0", ClampMax = "1"))
	float EdgeSoftness = 0.25f;

	/** Réglage propre à la forme (voir l'infobulle de chaque forme) : pointu/rond, douceur de rampe, rebord… */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon", meta = (ClampMin = "0", ClampMax = "1"))
	float Shaping = 0.5f;

	/**
	 * Rampe : relie automatiquement la hauteur du terrain sous son début et sous sa fin (ce qu'il y a avant elle
	 * dans la pile : plateau, autre tampon…). Ignore la hauteur du tampon et le mode de mélange ; les extrémités
	 * ne sont pas fondues, seulement les côtés. Façonnage à 1 = départ et arrivée à plat.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon", meta = (EditCondition = "Shape == EPatchTerrainStampShape::Ramp", EditConditionHides))
	bool bAutoFitEnds = true;

	/** Nombre de motifs (Boîte à œufs, Ondulations, Dunes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon", meta = (ClampMin = "0.1", ClampMax = "50"))
	float Frequency = 3.f;

	/**
	 * Formule (forme Expression). Résultat normalisé : 1 = la hauteur du tampon.
	 * Variables : u, v (0..1 sur l'emprise), x, y (cm, terrain), z (hauteur actuelle).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tampon", meta = (EditCondition = "Shape == EPatchTerrainStampShape::Expression", EditConditionHides, MultiLine = true))
	FString Expression = TEXT("sin(u*pi) * sin(v*pi)");

	UPROPERTY(VisibleAnywhere, Transient, Category = "Tampon", meta = (EditCondition = "Shape == EPatchTerrainStampShape::Expression", EditConditionHides))
	FString ExpressionStatus;

	// ---------------------------------------------------------------- Détails

	/** Marches douces appliquées à la forme (0 = aucune). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Détails", meta = (ClampMin = "0", ClampMax = "50"))
	int32 StepCount = 0;

	/** 0 = marches nettes, 1 = marches très fondues. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Détails", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "StepCount > 0"))
	float StepSoftness = 0.5f;

	/** Bruit ajouté à la forme ; il est ancré au tampon et le suit quand on le déplace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Détails")
	EPatchTerrainStampNoise NoiseType = EPatchTerrainStampNoise::None;

	/** Intensité du bruit, en fraction de la hauteur du tampon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Détails", meta = (ClampMin = "-2", ClampMax = "2", EditCondition = "NoiseType != EPatchTerrainStampNoise::None"))
	float NoiseAmount = 0.2f;

	/** Taille des motifs de bruit (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Détails", meta = (ClampMin = "10", Units = "cm", EditCondition = "NoiseType != EPatchTerrainStampNoise::None"))
	float NoiseScale = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Détails", meta = (ClampMin = "1", ClampMax = "10", EditCondition = "NoiseType != EPatchTerrainStampNoise::None"))
	int32 NoiseOctaves = 4;

	/** Change le motif du bruit de ce tampon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Détails")
	int32 Seed = 0;

	// ---------------------------------------------------------------- Ciblage

	/** Terrain affecté. Vide = tous les terrains du niveau. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ciblage")
	TObjectPtr<APatchTerrain> TargetTerrain;

	/** Ordre d'application : les priorités basses passent d'abord. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ciblage")
	int32 Priority = 0;

	/** Renomme automatiquement l'acteur selon sa forme (Stamp_Colline…). */
	UPROPERTY(EditAnywhere, Category = "Ciblage")
	bool bAutoLabel = true;

	// ---------------------------------------------------------------- API

	bool AffectsTerrain(const APatchTerrain* Terrain) const;

	/** Description du tampon dans l'espace local du terrain donné. */
	FPatchTerrainStampDesc MakeDesc(const FTransform& TerrainTransform) const;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/** Demande aux terrains concernés de se reconstruire. */
	void NotifyTerrains();

	UPROPERTY(VisibleAnywhere, Category = "Tampon")
	TObjectPtr<UBoxComponent> Bounds;
};
