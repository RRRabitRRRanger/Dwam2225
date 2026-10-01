#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PatchTerrainSurface.h"
#include "PatchTerrainLattice.generated.h"

class APatchTerrain;
class UBoxComponent;

/**
 * Lattice : boîte de déformation 3D (FFD de Bézier) appliquée en dernier sur le terrain.
 * On déplace ses points de contrôle dans le viewport ; tout ce qui est dans la boîte suit, en douceur.
 * C'est le moyen de faire des surplombs, des parois penchées, des arches légères.
 *
 * Boîte : 10 m × 10 m × 5 m à l'échelle 1 (X/Y centrés, Z de 0 à 5 m), comme les tampons.
 */
UCLASS(ClassGroup = Terrain, meta = (DisplayName = "Patch Terrain Lattice"))
class PATCHTERRAIN_API APatchTerrainLattice : public AActor
{
	GENERATED_BODY()

public:
	APatchTerrainLattice();

	static constexpr double BaseHalfSize = 500.0;
	static constexpr double BaseHeight = 500.0;

	/** Nombre de points de contrôle par axe (2 à 5). Changer la résolution remet la lattice au repos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lattice")
	FIntVector Resolution = FIntVector(3, 3, 3);

	/** Points de contrôle (espace local de l'acteur), déplaçables dans le viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lattice", meta = (MakeEditWidget))
	TArray<FVector> ControlPoints;

	/** Fondu de la déformation près des faces de la boîte (fraction de la boîte, 0 = aucun : gare aux marches). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lattice", meta = (ClampMin = "0", ClampMax = "0.5"))
	float Softness = 0.15f;

	/** Terrain affecté. Vide = tous les terrains du niveau. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ciblage")
	TObjectPtr<APatchTerrain> TargetTerrain;

	/** Remet tous les points au repos (aucune déformation). */
	UFUNCTION(CallInEditor, Category = "Lattice")
	void ResetLattice();

	bool AffectsTerrain(const APatchTerrain* Terrain) const;
	FPatchTerrainLatticeDesc MakeDesc(const FTransform& TerrainTransform) const;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

private:
	FIntVector GetClampedResolution() const;
	void NotifyTerrains();

	UPROPERTY(VisibleAnywhere, Category = "Lattice")
	TObjectPtr<UBoxComponent> Bounds;
};
