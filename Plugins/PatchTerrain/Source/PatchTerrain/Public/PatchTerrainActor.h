#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PatchTerrainTypes.h"
#include "PatchTerrainSurface.h"
#include "PatchTerrainActor.generated.h"

class UDynamicMeshComponent;
class UMaterialInterface;

/**
 * Terrain défini par un canevas de points de contrôle (surface lisse qui passe par les points),
 * plus une pile de modificateurs, maillé par chunks.
 *
 * Le terrain n'est jamais sauvegardé sous forme de mesh : il est toujours recalculé depuis ses données,
 * en éditeur (à chaque modification) comme en jeu (au BeginPlay), ce qui permet les variations par seed en package.
 */
UCLASS(ClassGroup = Terrain, meta = (DisplayName = "Patch Terrain"))
class PATCHTERRAIN_API APatchTerrain : public AActor
{
	GENERATED_BODY()

public:
	APatchTerrain();

	// ---------------------------------------------------------------- Canevas

	/** Largeur de chaque colonne de cases (cm). Le nombre d'éléments = nombre de colonnes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canevas", meta = (ClampMin = "1"))
	TArray<float> ColumnWidths;

	/** Profondeur de chaque ligne de cases (cm). Le nombre d'éléments = nombre de lignes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canevas", meta = (ClampMin = "1"))
	TArray<float> RowHeights;

	/**
	 * Points de contrôle (espace local), un par nœud de la grille, rangés ligne par ligne :
	 * index = Ligne * (NbColonnes + 1) + Colonne. Chacun est un handle déplaçable dans le viewport.
	 * Si la taille ne correspond plus à la grille, ils sont réinitialisés à plat.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canevas", meta = (MakeEditWidget))
	TArray<FVector> ControlPoints;

	/** Remet tous les points de contrôle à plat, sur la grille. */
	UFUNCTION(CallInEditor, Category = "Canevas")
	void ResetCanvas();

	/** Recale X/Y des points sur la grille (après un changement de largeurs), en gardant leur hauteur. */
	UFUNCTION(CallInEditor, Category = "Canevas")
	void RealignToGrid();

	// ---------------------------------------------------------------- Formules

	/** Pile de modificateurs, appliqués dans l'ordre sur la hauteur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formules")
	TArray<FPatchTerrainModifier> Modifiers;

	/** Seed des motifs aléatoires (bruits, dunes…). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formules")
	int32 Seed = 1337;

	/** En jeu : tire une seed aléatoire au lancement (test des variations en package). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formules")
	bool bRandomSeedAtRuntime = false;

	// ---------------------------------------------------------------- Maillage

	/** Taille d'un chunk, en nombre de cases (colonnes, lignes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maillage", meta = (ClampMin = "1"))
	FIntPoint ChunkCells = FIntPoint(4, 4);

	/** Écart entre sommets de la grille de base (cm), avant subdivision adaptative. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maillage", meta = (ClampMin = "10", Units = "cm"))
	float SampleSpacing = 200.f;

	/**
	 * Subdivisions maximales d'un carré de base là où le relief l'exige (0 = grille régulière).
	 * Le plus petit écart entre sommets vaut Sample Spacing / 2^N.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maillage", meta = (ClampMin = "0", ClampMax = "5"))
	int32 AdaptiveMaxDepth = 3;

	/** Écart toléré (cm) entre la surface réelle et le maillage avant de subdiviser. Plus petit = plus fidèle et plus lourd. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maillage", meta = (ClampMin = "0.5", Units = "cm"))
	float AdaptiveTolerance = 5.f;

	/** Taille couverte par une répétition d'UV (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maillage", meta = (ClampMin = "1", Units = "cm"))
	float UVTileSize = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maillage")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maillage")
	bool bGenerateCollision = true;

	/** Reconstruit automatiquement à chaque modification en éditeur. */
	UPROPERTY(EditAnywhere, Category = "Maillage")
	bool bAutoRebuildInEditor = true;

	// ---------------------------------------------------------------- API

	/** Recalcule tous les chunks. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain")
	void Rebuild();

	/**
	 * Demande une reconstruction. En éditeur, elle est différée à la frame suivante :
	 * plusieurs demandes dans la même frame (drag d'un tampon, chargement du niveau) n'en font qu'une.
	 */
	void RequestRebuild();

	/** Change la seed et reconstruit (utilisable en jeu). */
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	void RegenerateWithSeed(int32 NewSeed);

	/**
	 * Point de la surface (espace monde) aux coordonnées du canevas (cm, espace local du terrain).
	 * Retourne false si le terrain n'est pas encore construit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	bool GetSurfacePoint(FVector2D CanvasPosition, FVector& OutWorldPosition, FVector& OutWorldNormal) const;

	/** Dimensions totales du canevas (cm). */
	UFUNCTION(BlueprintPure, Category = "Terrain")
	FVector2D GetCanvasSize() const;

	int32 GetNumColumns() const { return ColumnWidths.Num(); }
	int32 GetNumRows() const { return RowHeights.Num(); }

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

private:
	void BuildKnots(TArray<double>& OutKnotsX, TArray<double>& OutKnotsY) const;
	void EnsureControlPoints();
	void ResetControlPoints();
	int32 GetEffectiveSeed() const;
	UDynamicMeshComponent* CreateChunkComponent();
	void DestroyChunkComponents();

	/** Composants générés (un par chunk) : transitoires, jamais sauvegardés. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDynamicMeshComponent>> ChunkComponents;

	/** Surface utilisée par la dernière reconstruction (pour les requêtes). */
	TSharedPtr<FPatchTerrainSurface> CachedSurface;

	/** Une reconstruction différée est déjà programmée (éditeur). */
	bool bRebuildPending = false;

	/** Seed tirée au lancement quand bRandomSeedAtRuntime est actif. */
	TOptional<int32> RuntimeSeed;
};
