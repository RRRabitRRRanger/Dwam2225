#include "RopeSolver.h"

// ---------------------------------------------------------------- Formes

float FRopeShape::SignedDistance(const FVector& P, FVector& OutNormal) const
{
	const FVector Center = Transform.GetLocation();
	const FQuat Rotation = Transform.GetRotation();

	switch (Type)
	{
	case ERopeShapeType::Sphere:
	{
		const FVector V = P - Center;
		const float Len = V.Size();
		OutNormal = Len > UE_KINDA_SMALL_NUMBER ? V / Len : FVector::UpVector;
		return Len - Radius;
	}

	case ERopeShapeType::Capsule:
	{
		// Segment intérieur (sans les hémisphères), puis distance à ce segment
		const FVector Axis = Rotation.GetAxisZ();
		const float Inner = FMath::Max(HalfHeight - Radius, 0.f);
		const float T = FMath::Clamp(FVector::DotProduct(P - Center, Axis), -Inner, Inner);
		const FVector Closest = Center + Axis * T;
		const FVector V = P - Closest;
		const float Len = V.Size();
		OutNormal = Len > UE_KINDA_SMALL_NUMBER ? V / Len : Rotation.GetAxisX();
		return Len - Radius;
	}

	case ERopeShapeType::Box:
	default:
	{
		const FVector Local = Rotation.UnrotateVector(P - Center);
		const FVector Q = Local.GetAbs() - Extent;
		if (Q.X > 0.f || Q.Y > 0.f || Q.Z > 0.f)
		{
			// Dehors : vers le point le plus proche de la boîte
			const FVector Clamped(
				FMath::Clamp(Local.X, -Extent.X, Extent.X),
				FMath::Clamp(Local.Y, -Extent.Y, Extent.Y),
				FMath::Clamp(Local.Z, -Extent.Z, Extent.Z));
			const FVector V = Local - Clamped;
			const float Len = V.Size();
			OutNormal = Rotation.RotateVector(Len > UE_KINDA_SMALL_NUMBER ? V / Len : FVector::UpVector);
			return Len;
		}
		// Dedans : sortir par la face la plus proche
		int32 Axis = 0;
		if (Q.Y > Q[Axis]) { Axis = 1; }
		if (Q.Z > Q[Axis]) { Axis = 2; }
		FVector LocalNormal = FVector::ZeroVector;
		LocalNormal[Axis] = Local[Axis] >= 0.f ? 1.f : -1.f;
		OutNormal = Rotation.RotateVector(LocalNormal);
		return Q[Axis];
	}
	}
}

// ---------------------------------------------------------------- Initialisation

void FRopeSolver::Init(const FVector& Start, const FVector& End, int32 NumSegments, float Length)
{
	NumSegments = FMath::Max(1, NumSegments);
	TArray<FVector> Points;
	Points.SetNum(NumSegments + 1);
	for (int32 I = 0; I <= NumSegments; ++I)
	{
		Points[I] = FMath::Lerp(Start, End, float(I) / NumSegments);
	}
	InitFromPoints(Points);
	SetTotalRestLength(Length);
}

void FRopeSolver::InitFromPoints(TConstArrayView<FVector> Points)
{
	check(Points.Num() >= 2);
	Positions = TArray<FVector>(Points.GetData(), Points.Num());
	Previous = Positions;
	InvMass.Init(1.f, Points.Num());
	ContactNormals.Init(FVector::ZeroVector, Points.Num());
	SetTotalRestLength(GetCurrentLength());
}

void FRopeSolver::SetTotalRestLength(float Length)
{
	SegmentRestLength = FMath::Max(Length, 0.1f) / FMath::Max(1, GetNumSegments());
}

float FRopeSolver::GetCurrentLength() const
{
	float Sum = 0.f;
	for (int32 I = 1; I < Positions.Num(); ++I)
	{
		Sum += FVector::Distance(Positions[I - 1], Positions[I]);
	}
	return Sum;
}

void FRopeSolver::SetPin(int32 Index, bool bPinned, const FVector& Position)
{
	if (!Positions.IsValidIndex(Index))
	{
		return;
	}
	InvMass[Index] = bPinned ? 0.f : 1.f;
	if (bPinned)
	{
		Previous[Index] = Positions[Index];
		Positions[Index] = Position;
	}
}

// ---------------------------------------------------------------- Pas de simulation

void FRopeSolver::SolveDistances(float Stiffness)
{
	for (int32 I = 0; I + 1 < Positions.Num(); ++I)
	{
		const float W0 = InvMass[I];
		const float W1 = InvMass[I + 1];
		const float WSum = W0 + W1;
		if (WSum <= 0.f)
		{
			continue;
		}
		const FVector Delta = Positions[I + 1] - Positions[I];
		const float Len = Delta.Size();
		if (Len < UE_KINDA_SMALL_NUMBER)
		{
			continue;
		}
		// Chaque segment reprend sa longueur ; la correction est partagée selon les masses
		const FVector Correction = Delta * ((Len - SegmentRestLength) / Len * Stiffness / WSum);
		Positions[I] += Correction * W0;
		Positions[I + 1] -= Correction * W1;
	}
}

void FRopeSolver::SolveShapes(TConstArrayView<FRopeShape> Shapes, float NodeRadius)
{
	for (int32 I = 0; I < Positions.Num(); ++I)
	{
		if (InvMass[I] <= 0.f)
		{
			continue;
		}
		for (const FRopeShape& Shape : Shapes)
		{
			FVector Normal;
			const float Distance = Shape.SignedDistance(Positions[I], Normal);
			if (Distance < NodeRadius)
			{
				Positions[I] += Normal * (NodeRadius - Distance);
				ContactNormals[I] = Normal;
			}
		}
	}
}

void FRopeSolver::Step(float Dt, const FRopeSolverParams& Params, TConstArrayView<FRopeShape> Shapes, const FWorldSweep* WorldSweep)
{
	const int32 N = Positions.Num();
	if (N < 2 || Dt <= 0.f)
	{
		return;
	}

	for (FVector& Normal : ContactNormals)
	{
		Normal = FVector::ZeroVector;
	}

	// 1. Verlet : inertie + gravité
	const FVector GravityStep = Params.Gravity * (Dt * Dt);
	const float Keep = 1.f - FMath::Clamp(Params.Damping, 0.f, 1.f);
	for (int32 I = 0; I < N; ++I)
	{
		if (InvMass[I] <= 0.f)
		{
			continue;
		}
		const FVector Velocity = (Positions[I] - Previous[I]) * Keep;
		Previous[I] = Positions[I];
		Positions[I] += Velocity + GravityStep;
	}

	// 2. Décor : on arrête chaque nœud au premier contact sur son trajet
	TArray<FVector, TInlineAllocator<64>> AfterSweep;
	if (WorldSweep)
	{
		for (int32 I = 0; I < N; ++I)
		{
			FVector Hit, Normal;
			if (InvMass[I] > 0.f && (*WorldSweep)(Previous[I], Positions[I], Hit, Normal))
			{
				Positions[I] = Hit;
				ContactNormals[I] = Normal;
			}
		}
		AfterSweep = TArray<FVector, TInlineAllocator<64>>(Positions.GetData(), N);
	}

	// 3. Contraintes : segments, puis formes analytiques, répétés
	for (int32 Iter = 0; Iter < Params.Iterations; ++Iter)
	{
		SolveDistances(Params.Stiffness);
		SolveShapes(Shapes, Params.NodeRadius);
		if (Params.bSelfCollision)
		{
			SolveSelfCollision(Params.NodeRadius);
		}
	}

	// 4. Décor encore : les contraintes ne doivent pas tirer un nœud à travers un mur
	if (WorldSweep)
	{
		for (int32 I = 0; I < N; ++I)
		{
			FVector Hit, Normal;
			if (InvMass[I] > 0.f && (*WorldSweep)(AfterSweep[I], Positions[I], Hit, Normal))
			{
				Positions[I] = Hit;
				ContactNormals[I] = Normal;
			}
		}
	}

	// 5. Frottement : au contact, on retire une partie du glissement tangentiel du sous-pas
	const float Friction = FMath::Clamp(Params.Friction, 0.f, 1.f);
	if (Friction > 0.f)
	{
		for (int32 I = 0; I < N; ++I)
		{
			const FVector& Normal = ContactNormals[I];
			if (InvMass[I] <= 0.f || Normal.IsZero())
			{
				continue;
			}
			const FVector Displacement = Positions[I] - Previous[I];
			const FVector Tangential = Displacement - Normal * FVector::DotProduct(Displacement, Normal);
			Positions[I] -= Tangential * Friction;
		}
	}
}

void FRopeSolver::SolveSelfCollision(float NodeRadius)
{
	// Deux nœuds non voisins restent à au moins deux rayons. On saute les voisins proches le long
	// de la corde (leur distance de repos est déjà inférieure à deux rayons).
	const float MinDistance = 2.f * NodeRadius;
	const float MinDistanceSq = MinDistance * MinDistance;
	const int32 Skip = FMath::Max(2, FMath::CeilToInt(MinDistance * 1.5f / FMath::Max(SegmentRestLength, 0.01f)));
	const int32 N = Positions.Num();

	for (int32 I = 0; I < N; ++I)
	{
		for (int32 J = I + Skip; J < N; ++J)
		{
			const FVector Delta = Positions[J] - Positions[I];
			const float DistSq = Delta.SizeSquared();
			if (DistSq >= MinDistanceSq || DistSq < UE_SMALL_NUMBER)
			{
				continue;
			}
			const float WSum = InvMass[I] + InvMass[J];
			if (WSum <= 0.f)
			{
				continue;
			}
			const float Dist = FMath::Sqrt(DistSq);
			const FVector Correction = Delta * ((MinDistance - Dist) / Dist / WSum);
			Positions[I] -= Correction * InvMass[I];
			Positions[J] += Correction * InvMass[J];
		}
	}
}
