#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SnakeToy.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/**
 * Jouet : un « snake » fait de sphères qui rampe sur les surfaces de la scène.
 *
 *  - La tête suit les surfaces par traces CPU complexes (géométrie visuelle) : elle grimpe sur un mur
 *    rencontré, contourne une arête, retombe si elle perd le sol, et erre au hasard (bruit).
 *  - Le corps suit le chemin parcouru par la tête.
 *  - Chaque sphère reçoit ses voisines dans ses Custom Primitive Data, pour le transfert de normales
 *    analytique (DwamChainNormal dans /Plugin/DwamLighting/NormalTransfer.ush) :
 *      floats 0-3  : voisine précédente (offset X, Y, Z relatif au centre, rayon ; 0 = aucune)
 *      floats 4-7  : voisine suivante   (offset X, Y, Z, rayon ; 0 = aucune)
 *      floats 8-11 : rayon propre, lissage K (cm), 0, 0
 */
UCLASS(meta = (DisplayName = "Snake Toy"))
class DWAM2225_API ASnakeToy : public AActor
{
	GENERATED_BODY()

public:
	ASnakeToy();

	/** Nombre de sphères. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "2", ClampMax = "128"))
	int32 NumSegments = 24;

	/** Rayon de la tête (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "1", Units = "cm"))
	float HeadRadius = 15.f;

	/** Rayon du bout de la queue, en fraction du rayon de la tête. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "0.05", ClampMax = "1"))
	float TailRadiusRatio = 0.35f;

	/** Écart entre deux centres, en fraction de la somme de leurs rayons (< 1 = les sphères se chevauchent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "0.1", ClampMax = "2"))
	float SpacingRatio = 0.6f;

	/** Vitesse de la tête (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "0"))
	float Speed = 150.f;

	/** Force de l'errance (vitesse de rotation max, degrés par seconde). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "0"))
	float WanderStrength = 90.f;

	/** Fréquence de l'errance (changements de direction par seconde, environ). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "0"))
	float WanderFrequency = 0.4f;

	/** Largeur du raccord lisse entre sphères, pour le matériau (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu", meta = (ClampMin = "0", Units = "cm"))
	float BlendK = 8.f;

	/** Matériau des sphères (voir Docs/Guide_NormalTransfer.md). Vide = matériau par défaut, sans lissage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu")
	TObjectPtr<UMaterialInterface> Material;

	/** Temps de grâce sans sol avant de tomber (s) : évite le clignotement sur les murs quand une trace rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake", meta = (ClampMin = "0", ClampMax = "2"))
	float CoyoteTime = 0.2f;

	/** Canal utilisé pour les traces de surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Dessine les traces et l'orientation de la tête. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebug = false;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	/** Point du chemin de la tête (le plus récent en premier). */
	struct FPathPoint
	{
		FVector Position;
		FVector Up;
	};

	void EnsureSegments();
	float GetSegmentRadius(int32 Index) const;
	void ResetPath(const FVector& HeadPosition, const FVector& Up, const FVector& Forward);
	void CrawlHead(float DeltaSeconds);
	void PushPathPoint(const FVector& Position, const FVector& Up);
	bool SamplePath(float Distance, FVector& OutPosition, FVector& OutUp) const;
	void UpdateSegments();
	bool Trace(const FVector& Start, const FVector& End, FHitResult& OutHit) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Segments;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	TArray<FPathPoint> Path;
	FVector HeadPos = FVector::ZeroVector;
	FVector HeadUp = FVector::UpVector;
	FVector HeadFwd = FVector::ForwardVector;
	float FallSpeed = 0.f;
	/** Temps passé sans sol sous la tête (temps de grâce avant la chute). */
	float AirTime = 0.f;
	float Time = 0.f;
};
