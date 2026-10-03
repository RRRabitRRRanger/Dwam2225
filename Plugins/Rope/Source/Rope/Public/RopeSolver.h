#pragma once

#include "CoreMinimal.h"

/** Forme analytique contre laquelle la corde collisionne. */
enum class ERopeShapeType : uint8
{
	Sphere,
	/** Axe = Z local ; HalfHeight inclut les hémisphères (convention Unreal). */
	Capsule,
	/** Boîte orientée ; Extent = demi-dimensions. */
	Box
};

struct FRopeShape
{
	ERopeShapeType Type = ERopeShapeType::Sphere;
	FTransform Transform;   // échelle ignorée
	float Radius = 50.f;
	float HalfHeight = 100.f;
	FVector Extent = FVector(50.f);

	/** Distance signée de P à la surface (négative à l'intérieur) et normale sortante. */
	ROPE_API float SignedDistance(const FVector& P, FVector& OutNormal) const;
};

struct FRopeSolverParams
{
	/** Itérations de contraintes par sous-pas (Gauss-Seidel). */
	int32 Iterations = 16;
	/** Amortissement de la vitesse par sous-pas (0..1). */
	float Damping = 0.01f;
	/** Raideur des segments (1 = inextensible). */
	float Stiffness = 1.f;
	/** Frottement au contact (0 = glisse, 1 = colle). */
	float Friction = 0.5f;
	/** Rayon de la corde (cm) : distance gardée aux obstacles. */
	float NodeRadius = 2.f;
	/** La corde se repousse elle-même (nœuds non voisins) : les tours d'un enroulement ne se traversent pas. Coût en N². */
	bool bSelfCollision = false;
	FVector Gravity = FVector(0.f, 0.f, -980.f);
};

/**
 * Corde : chaîne de nœuds, intégration de Verlet + contraintes de distance (Position Based Dynamics,
 * Müller et al. 2007), épingles, collisions analytiques (sphère, capsule, boîte) et décor (balayages
 * fournis par l'appelant), frottement au contact.
 *
 * Sans UObject : testable hors du jeu. Espace monde, centimètres.
 */
class ROPE_API FRopeSolver
{
public:
	/**
	 * Balayage d'une sphère dans le décor, de From à To. Retourne true au contact, avec la position
	 * du centre au contact et la normale. Fourni par le composant (SweepSingleByChannel).
	 */
	using FWorldSweep = TFunction<bool(const FVector& From, const FVector& To, FVector& OutPosition, FVector& OutNormal)>;

	/** Corde droite de Start à End, NumSegments segments, longueur de repos totale Length. */
	void Init(const FVector& Start, const FVector& End, int32 NumSegments, float Length);

	/** Place les nœuds sur une liste de points (ex. hélice), longueur de repos = longueur de la polyligne. */
	void InitFromPoints(TConstArrayView<FVector> Points);

	/** Épingle (ou libère) un nœud à une position. Une épingle se déplace sans inertie. */
	void SetPin(int32 Index, bool bPinned, const FVector& Position);

	/** Avance d'un sous-pas. */
	void Step(float Dt, const FRopeSolverParams& Params, TConstArrayView<FRopeShape> Shapes, const FWorldSweep* WorldSweep = nullptr);

	/** Longueur de repos totale (pour serrer ou relâcher la corde). */
	void SetTotalRestLength(float Length);
	float GetTotalRestLength() const { return SegmentRestLength * GetNumSegments(); }

	int32 GetNumNodes() const { return Positions.Num(); }
	int32 GetNumSegments() const { return FMath::Max(0, Positions.Num() - 1); }
	const TArray<FVector>& GetPositions() const { return Positions; }
	bool IsInContact(int32 Index) const { return !ContactNormals[Index].IsZero(); }

	/** Longueur actuelle de la polyligne (pour mesurer l'étirement). */
	float GetCurrentLength() const;

private:
	TArray<FVector> Positions;
	TArray<FVector> Previous;
	TArray<float> InvMass;
	TArray<FVector> ContactNormals;
	float SegmentRestLength = 10.f;

	void SolveDistances(float Stiffness);
	void SolveShapes(TConstArrayView<FRopeShape> Shapes, float NodeRadius);
	void SolveSelfCollision(float NodeRadius);
};
