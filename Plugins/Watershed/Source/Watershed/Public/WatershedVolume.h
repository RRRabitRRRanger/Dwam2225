#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Async/Future.h"
#include "WatershedHierarchy.h"
#include "WatershedVolume.generated.h"

class UBoxComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/** Source d'eau ponctuelle (débit constant). */
USTRUCT(BlueprintType)
struct WATERSHED_API FWatershedSource
{
	GENERATED_BODY()

	/** Position (relative à l'acteur). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source", meta = (MakeEditWidget = true))
	FVector Location = FVector::ZeroVector;

	/** Débit (m³/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source", meta = (ClampMin = "0"))
	float FlowRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source")
	bool bEnabled = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWatershedEvent);

/**
 * Eau par bassins (plan : Docs/Plan_Water.md).
 *
 * La boîte de l'acteur délimite la région. Au Rebuild :
 *  1. la hauteur du sol est échantillonnée par des traces verticales, une par cellule (2 m) ;
 *  2. la hiérarchie de bassins est construite (tâche de fond en jeu) ;
 *  3. l'eau déjà versée est redistribuée ;
 *  4. les lacs sont rendus en tuiles carrées (quadtree implicite) : grandes au large, petites au rivage.
 *
 * L'eau se verse par les sources (débit), la pluie, ou AddWaterAtLocation (Blueprint).
 * Rien n'est simulé au repos : un lac ne coûte rien tant qu'on n'y verse rien.
 */
UCLASS(meta = (DisplayName = "Watershed Volume"))
class WATERSHED_API AWatershedVolume : public AActor
{
	GENERATED_BODY()

public:
	AWatershedVolume();

	// ---------------------------------------------------------------- Composants

	/** Région couverte (X, Y) et plage des traces (Z). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Watershed")
	TObjectPtr<UBoxComponent> Bounds;

	/** Tuiles des lacs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Watershed")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WaterTiles;

	// ---------------------------------------------------------------- Grille

	/** Côté d'une cellule (cm). 2 m par défaut et minimum (décision du plan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Grid", meta = (ClampMin = "200", Units = "Centimeters"))
	float CellSize = 200.f;

	/** Les bords de la région évacuent l'eau. Décoché : la région est une cuve fermée. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Grid")
	bool bEdgesDrain = true;

	/** Canal des traces de hauteur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Grid")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Traces complexes (géométrie de rendu) : nécessaire pour un mesh sans collision simple. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Grid")
	bool bTraceComplex = true;

	/** Acteurs ignorés par les traces (ex. un plan d'eau existant). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Grid")
	TArray<TObjectPtr<AActor>> IgnoredActors;

	/** Garde-fou : nombre maximal de cellules (traces faites sur le thread de jeu). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Grid", meta = (ClampMin = "1024"))
	int32 MaxCells = 262144;

	/** Reconstruit au début du jeu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Grid")
	bool bBuildOnBeginPlay = true;

	// ---------------------------------------------------------------- Eau

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watershed|Water")
	TArray<FWatershedSource> Sources;

	/** Pluie versée au début du jeu, sur toute la région (mm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watershed|Water", meta = (ClampMin = "0", Units = "Millimeters"))
	float InitialRain = 0.f;

	// ---------------------------------------------------------------- Rendu

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Rendering")
	TObjectPtr<UMaterialInterface> WaterMaterial;

	/** Mesh d'une tuile : un plan de 100 × 100 cm centré, face +Z (par défaut /Engine/BasicShapes/Plane). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Rendering")
	TObjectPtr<UStaticMesh> TileMesh;

	/** Taille max d'une tuile : 2^N cellules de côté. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Rendering", meta = (ClampMin = "0", ClampMax = "10"))
	int32 MaxTileLevel = 5;

	/** Profondeur d'eau en dessous de laquelle une cellule est considérée sèche (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Rendering", meta = (ClampMin = "0", Units = "Centimeters"))
	float MinDepth = 1.f;

	/** Intervalle minimal entre deux reconstructions des tuiles quand l'eau bouge (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watershed|Rendering", meta = (ClampMin = "0", Units = "Seconds"))
	float TileUpdateInterval = 0.25f;

	// ---------------------------------------------------------------- Debug

	/** Cols (sphères), niveaux des lacs (texte), débordements (flèches). Visible aussi en éditeur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watershed|Debug")
	bool bDebugDraw = false;

	/** Affiche aussi les hauteurs échantillonnées (un point par cellule ; coûteux sur une grande région). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Watershed|Debug")
	bool bDebugDrawHeights = false;

	// ---------------------------------------------------------------- API

	/** Échantillonne le sol et reconstruit la hiérarchie (asynchrone en jeu). L'eau versée est conservée. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Watershed")
	void Rebuild();

	/**
	 * Mise à jour locale : rééchantillonne seulement les cellules dans la boîte (monde), puis reconstruit
	 * la hiérarchie. À appeler quand un obstacle apparaît ou que le terrain change.
	 */
	UFUNCTION(BlueprintCallable, Category = "Watershed")
	void RebuildRegion(const FBox& WorldBox);

	/** Verse un volume d'eau (m³) à une position (monde). Retourne le volume perdu vers l'extérieur (m³). */
	UFUNCTION(BlueprintCallable, Category = "Watershed")
	float AddWaterAtLocation(FVector WorldLocation, float VolumeM3);

	/** Pluie uniforme sur toute la région (mm). */
	UFUNCTION(BlueprintCallable, Category = "Watershed")
	void AddRain(float DepthMM);

	/** Test en éditeur : 100 mm de pluie. */
	UFUNCTION(CallInEditor, Category = "Watershed")
	void AddTestRain() { AddRain(100.f); }

	/** Vide toute l'eau. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Watershed")
	void ClearWater();

	/**
	 * Niveau de l'eau à une position (monde).
	 * @return true si la position est dans un lac (profondeur > 0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Watershed")
	bool GetWaterAtLocation(FVector WorldLocation, float& OutLevel, float& OutDepth) const;

	/** Nombre de lacs contenant de l'eau. */
	UFUNCTION(BlueprintPure, Category = "Watershed")
	int32 GetNumWetLakes() const;

	/** Appelé quand une hiérarchie est prête (après Rebuild). */
	UPROPERTY(BlueprintAssignable, Category = "Watershed")
	FWatershedEvent OnHierarchyBuilt;

	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return bDebugDraw; }

protected:
	virtual void BeginPlay() override;

private:
	TSharedPtr<FWatershedHierarchy, ESPMode::ThreadSafe> Hierarchy;
	TFuture<TSharedPtr<FWatershedHierarchy, ESPMode::ThreadSafe>> PendingBuild;
	bool bBuildPending = false;
	bool bRebuildQueued = false;

	/** Grille échantillonnée (origine = coin min de la boîte, en monde). */
	FWatershedGridDesc GridDesc;
	FVector GridOrigin = FVector::ZeroVector;
	TArray<float> Heights;

	/** Eau versée par cellule (cm³), conservée d'une reconstruction à l'autre. */
	TArray<double> CellInputs;

	bool bTilesDirty = false;
	float TimeSinceTileUpdate = 0.f;

	bool ComputeGrid(FWatershedGridDesc& OutDesc, FVector& OutOrigin) const;
	void SampleHeights(int32 MinX, int32 MinY, int32 MaxX, int32 MaxY);
	void StartBuild();
	void OnBuildFinished(TSharedPtr<FWatershedHierarchy, ESPMode::ThreadSafe> NewHierarchy);
	void ReapplyInputs();
	void AddWaterToCell(int32 Cell, double VolumeCm3);
	void UpdateTiles();
	void DrawDebug() const;

	int32 CellAt(const FVector& WorldLocation) const;
	FVector CellCenter(int32 X, int32 Y) const;
};
