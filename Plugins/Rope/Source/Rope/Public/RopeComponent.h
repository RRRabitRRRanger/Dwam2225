#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "RopeSolver.h"
#include "RopeComponent.generated.h"

UENUM(BlueprintType)
enum class ERopeColliderShape : uint8
{
	Sphere,
	/** Axe vertical (Z local) ; Half Height inclut les hémisphères. */
	Capsule,
	Box
};

/** Point d'accroche d'une extrémité. */
USTRUCT(BlueprintType)
struct ROPE_API FRopeAnchor
{
	GENERATED_BODY()

	/** L'extrémité suit ce point. Décoché : elle pend librement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	bool bAttached = false;

	/** Acteur suivi. Vide : le point est dans l'espace du composant corde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	TObjectPtr<AActor> Actor;

	/** Socket ou os de l'acteur (cherché sur ses composants : main, ceinture...). Vide : son pivot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	FName Socket;

	/** Décalage dans l'espace du socket / de l'acteur / du composant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	FVector Offset = FVector::ZeroVector;
};

/** Obstacle analytique (précis et bon marché) : poteau, tonneau, tronc, membre du personnage... */
USTRUCT(BlueprintType)
struct ROPE_API FRopeCollider
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	ERopeColliderShape Shape = ERopeColliderShape::Capsule;

	/** Acteur suivi. Vide : forme fixe dans l'espace du monde (Offset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	TObjectPtr<AActor> Actor;

	/** Socket ou os de l'acteur (ex. « spine_03 » pour le torse). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	FName Socket;

	/** Place la forme dans l'espace du socket / de l'acteur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	FTransform Offset;

	/** Capsule verticale ajustée aux bornes de l'acteur (ignore Radius, Half Height et Offset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	bool bFitToActorBounds = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope", meta = (ClampMin = "0", Units = "Centimeters"))
	float Radius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope", meta = (ClampMin = "0", Units = "Centimeters"))
	float HalfHeight = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	FVector BoxExtent = FVector(50.f);
};

/**
 * Corde simulée sur CPU (plan : Docs/Plan_NiagaraRope.md, variante CPU) et rendue en tube.
 *
 * Verlet + contraintes de distance (PBD), épingles aux extrémités, collisions avec des formes
 * analytiques (Colliders) et le décor (balayages), frottement au contact. Peut s'enrouler autour d'un
 * obstacle (WrapAround) puis se serrer (TightenTo).
 *
 * Le composant est lui-même un Dynamic Mesh : poser le composant, choisir un matériau, c'est tout.
 * Pour un rendu Niagara (ruban), GetNodePositions donne les nœuds, à passer à un tableau Niagara.
 */
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent, DisplayName = "Rope"))
class ROPE_API URopeComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	URopeComponent();

	// ---------------------------------------------------------------- Forme

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope", meta = (ClampMin = "1", ClampMax = "256"))
	int32 NumSegments = 32;

	/** Longueur de repos (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope", meta = (ClampMin = "1", Units = "Centimeters"))
	float Length = 400.f;

	/** Rayon de la corde (cm) : épaisseur du tube et distance gardée aux obstacles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope", meta = (ClampMin = "0.1", Units = "Centimeters"))
	float Thickness = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	FRopeAnchor StartAnchor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	FRopeAnchor EndAnchor;

	// ---------------------------------------------------------------- Simulation

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation", meta = (ClampMin = "1", ClampMax = "16"))
	int32 SubSteps = 2;

	/** Itérations des contraintes par sous-pas : plus = moins élastique. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation", meta = (ClampMin = "1", ClampMax = "128"))
	int32 Iterations = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation", meta = (ClampMin = "0", ClampMax = "1"))
	float Damping = 0.01f;

	/** 1 = inextensible, moins = élastique. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation", meta = (ClampMin = "0", ClampMax = "1"))
	float Stiffness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation")
	float GravityScale = 1.f;

	/** Frottement au contact : 0 = glisse, 1 = colle. Retient la corde enroulée. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation", meta = (ClampMin = "0", ClampMax = "1"))
	float Friction = 0.6f;

	/** Vitesse à laquelle la longueur rejoint la cible de TightenTo (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float TightenSpeed = 50.f;

	/** Simule aussi dans l'éditeur (aperçu sans lancer le jeu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Simulation")
	bool bSimulateInEditor = false;

	// ---------------------------------------------------------------- Collisions

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Collision")
	TArray<FRopeCollider> Colliders;

	/** Collision avec le décor (balayage d'une sphère par nœud et par sous-pas). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Collision")
	bool bCollideWithWorld = true;

	/** La corde se repousse elle-même : les tours d'un enroulement restent côte à côte. Coût en N² (garder NumSegments raisonnable, ~64). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Collision")
	bool bSelfCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Collision")
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECC_Visibility;

	/** Le décor ignore l'acteur propriétaire (souvent celui qui tient la corde). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope|Collision")
	bool bIgnoreOwner = true;

	// ---------------------------------------------------------------- Rendu

	/** Côtés du tube. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope|Rendering", meta = (ClampMin = "3", ClampMax = "32"))
	int32 Sides = 8;

	/** Génère le tube. Décoché : seulement la simulation (rendu par Niagara, par exemple). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope|Rendering")
	bool bGenerateMesh = true;

	// ---------------------------------------------------------------- API

	/** Remet la corde droite entre ses accroches. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Rope")
	void ResetRope();

	/**
	 * Enroule la corde en hélice autour d'un obstacle de la liste Colliders.
	 * La longueur de repos devient celle de l'hélice ; appeler ensuite TightenTo pour serrer.
	 * @param Turns     nombre de tours
	 * @param Spacing   écart vertical entre deux tours (cm) ; 0 = 3 × l'épaisseur
	 * @param bTieEnds  attache les deux bouts à l'obstacle (ils le suivent s'il bouge). Sans attache, une
	 *                  corde serrée se déroule, et une corde lâche glisse le long d'un poteau vertical.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rope")
	void WrapAround(int32 ColliderIndex = 0, float Turns = 3.f, float Spacing = 0.f, bool bTieEnds = true);

	/** Test en éditeur : 3 tours autour du premier obstacle, bouts attachés, puis serre à 97 %. */
	UFUNCTION(CallInEditor, Category = "Rope")
	void WrapAroundFirstCollider();

	/** Change la longueur de repos progressivement (TightenSpeed) : serrer ou relâcher. */
	UFUNCTION(BlueprintCallable, Category = "Rope")
	void TightenTo(float NewLength);

	/** Positions des nœuds (monde). */
	UFUNCTION(BlueprintPure, Category = "Rope")
	TArray<FVector> GetNodePositions() const { return Solver.GetPositions(); }

	/** Longueur actuelle (cm) : supérieure à la longueur de repos si la corde est étirée. */
	UFUNCTION(BlueprintPure, Category = "Rope")
	float GetCurrentLength() const { return Solver.GetCurrentLength(); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void OnRegister() override;

private:
	FRopeSolver Solver;
	float TargetLength = -1.f;
	int32 BuiltRings = 0;
	int32 BuiltSides = 0;

	bool ResolveAnchor(const FRopeAnchor& Anchor, FVector& OutPosition) const;
	bool ResolveCollider(const FRopeCollider& Collider, FRopeShape& OutShape) const;
	void ApplyPins();
	void BuildTubeTopology();
	void UpdateTube();
};
