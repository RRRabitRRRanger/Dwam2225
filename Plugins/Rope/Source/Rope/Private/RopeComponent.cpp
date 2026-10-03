#include "RopeComponent.h"

#include "Components/SceneComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

using UE::Geometry::FDynamicMesh3;
using UE::Geometry::FDynamicMeshNormalOverlay;
using UE::Geometry::FDynamicMeshUVOverlay;
using UE::Geometry::FIndex3i;

namespace RopePrivate
{
	/** Transform d'un socket / os de l'acteur (cherché sur tous ses composants), sinon de l'acteur. */
	FTransform GetActorSocketTransform(const AActor* Actor, FName Socket)
	{
		if (!Socket.IsNone())
		{
			TInlineComponentArray<USceneComponent*> Components(Actor);
			for (const USceneComponent* Component : Components)
			{
				if (Component && Component->DoesSocketExist(Socket))
				{
					return Component->GetSocketTransform(Socket);
				}
			}
		}
		return Actor->GetActorTransform();
	}
}

URopeComponent::URopeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Après l'animation et la physique : les os et les mains sont à leur place de la frame
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bTickInEditor = true;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StartAnchor.bAttached = true;
	EndAnchor.bAttached = false;
	EndAnchor.Offset = FVector(400.f, 0.f, 0.f);
}

void URopeComponent::OnRegister()
{
	Super::OnRegister();
	ResetRope();
}

#if WITH_EDITOR
void URopeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ResetRope();
}
#endif

// ---------------------------------------------------------------- Accroches et obstacles

bool URopeComponent::ResolveAnchor(const FRopeAnchor& Anchor, FVector& OutPosition) const
{
	if (!Anchor.bAttached)
	{
		return false;
	}
	if (Anchor.Actor)
	{
		OutPosition = RopePrivate::GetActorSocketTransform(Anchor.Actor, Anchor.Socket).TransformPosition(Anchor.Offset);
	}
	else
	{
		OutPosition = GetComponentTransform().TransformPosition(Anchor.Offset);
	}
	return true;
}

bool URopeComponent::ResolveCollider(const FRopeCollider& Collider, FRopeShape& OutShape) const
{
	if (Collider.bFitToActorBounds)
	{
		if (!Collider.Actor)
		{
			return false;
		}
		// Capsule verticale qui englobe les composants avec collision de l'acteur
		FVector Origin, Extent;
		Collider.Actor->GetActorBounds(true, Origin, Extent);
		OutShape.Type = ERopeShapeType::Capsule;
		OutShape.Transform = FTransform(Origin);
		OutShape.Radius = FMath::Max(Extent.X, Extent.Y);
		OutShape.HalfHeight = FMath::Max(Extent.Z, OutShape.Radius);
		return true;
	}

	const FTransform Base = Collider.Actor
		? RopePrivate::GetActorSocketTransform(Collider.Actor, Collider.Socket)
		: FTransform::Identity;
	OutShape.Transform = Collider.Offset * Base;
	OutShape.Radius = Collider.Radius;
	OutShape.HalfHeight = Collider.HalfHeight;
	OutShape.Extent = Collider.BoxExtent;
	switch (Collider.Shape)
	{
	case ERopeColliderShape::Sphere:  OutShape.Type = ERopeShapeType::Sphere; break;
	case ERopeColliderShape::Capsule: OutShape.Type = ERopeShapeType::Capsule; break;
	default:                          OutShape.Type = ERopeShapeType::Box; break;
	}
	return true;
}

void URopeComponent::ApplyPins()
{
	const int32 Last = Solver.GetNumNodes() - 1;
	if (Last < 1)
	{
		return;
	}
	FVector Position;
	Solver.SetPin(0, ResolveAnchor(StartAnchor, Position), Position);
	Solver.SetPin(Last, ResolveAnchor(EndAnchor, Position), Position);
}

// ---------------------------------------------------------------- API

void URopeComponent::ResetRope()
{
	FVector Start = GetComponentLocation();
	ResolveAnchor(StartAnchor, Start);

	FVector End;
	if (!ResolveAnchor(EndAnchor, End))
	{
		End = Start + GetForwardVector() * Length;
	}

	Solver.Init(Start, End, NumSegments, Length);
	TargetLength = -1.f;
	ApplyPins();
	BuiltRings = 0;
	UpdateTube();
}

void URopeComponent::WrapAround(int32 ColliderIndex, float Turns, float Spacing, bool bTieEnds)
{
	FRopeShape Shape;
	if (!Colliders.IsValidIndex(ColliderIndex) || !ResolveCollider(Colliders[ColliderIndex], Shape))
	{
		UE_LOG(LogTemp, Warning, TEXT("Rope : obstacle %d introuvable pour WrapAround."), ColliderIndex);
		return;
	}

	const int32 NumNodes = NumSegments + 1;
	Turns = FMath::Max(Turns, 0.1f);
	Spacing = Spacing > 0.f ? Spacing : 3.f * Thickness;

	// Rayon de l'hélice : la surface de l'obstacle + l'épaisseur, agrandi pour que les segments
	// (des cordes de cercle) ne traversent pas l'obstacle entre deux nœuds
	float SurfaceRadius = Shape.Radius;
	if (Shape.Type == ERopeShapeType::Box)
	{
		SurfaceRadius = FVector2D(Shape.Extent.X, Shape.Extent.Y).Size();
	}
	const float NodesPerTurn = FMath::Max(3.f, (NumNodes - 1) / Turns);
	const float HelixRadius = (SurfaceRadius + Thickness) / FMath::Cos(PI / NodesPerTurn);

	const FVector Center = Shape.Transform.GetLocation();
	const FVector Axis = Shape.Transform.GetRotation().GetAxisZ();
	FVector U, V;
	Axis.FindBestAxisVectors(U, V);

	TArray<FVector> Points;
	Points.SetNum(NumNodes);
	for (int32 I = 0; I < NumNodes; ++I)
	{
		const float T = float(I) / (NumNodes - 1);
		const float Angle = 2.f * PI * Turns * T;
		const float Height = (T - 0.5f) * Turns * Spacing;
		Points[I] = Center + Axis * Height + (U * FMath::Cos(Angle) + V * FMath::Sin(Angle)) * HelixRadius;
	}

	if (bTieEnds)
	{
		// Les bouts sont noués sur l'obstacle, à leur place dans l'hélice, dans l'espace de son acteur
		Modify();
		const FRopeCollider& Collider = Colliders[ColliderIndex];
		auto Tie = [this, &Collider](FRopeAnchor& Anchor, const FVector& WorldPosition)
		{
			Anchor.bAttached = true;
			if (Collider.Actor)
			{
				const FName Socket = Collider.bFitToActorBounds ? NAME_None : Collider.Socket;
				Anchor.Actor = Collider.Actor;
				Anchor.Socket = Socket;
				Anchor.Offset = RopePrivate::GetActorSocketTransform(Collider.Actor, Socket).InverseTransformPosition(WorldPosition);
			}
			else
			{
				Anchor.Actor = nullptr;
				Anchor.Socket = NAME_None;
				Anchor.Offset = GetComponentTransform().InverseTransformPosition(WorldPosition);
			}
		};
		Tie(StartAnchor, Points[0]);
		Tie(EndAnchor, Points.Last());
	}

	Solver.InitFromPoints(Points);
	TargetLength = -1.f;
	ApplyPins();
	BuiltRings = 0;
	UpdateTube();
}

void URopeComponent::WrapAroundFirstCollider()
{
	WrapAround(0, 3.f, 0.f, true);
	TightenTo(Solver.GetTotalRestLength() * 0.97f);
}

void URopeComponent::TightenTo(float NewLength)
{
	TargetLength = FMath::Max(NewLength, 1.f);
}

// ---------------------------------------------------------------- Simulation

void URopeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World || Solver.GetNumNodes() < 2)
	{
		return;
	}
	if (!World->IsGameWorld() && !bSimulateInEditor)
	{
		return;
	}

	// Évite l'explosion après un à-coup (chargement, point d'arrêt)
	const float FrameDt = FMath::Min(DeltaTime, 1.f / 30.f);

	// Serrage / relâchement progressif
	if (TargetLength > 0.f)
	{
		const float Current = Solver.GetTotalRestLength();
		const float Next = FMath::FInterpConstantTo(Current, TargetLength, FrameDt, TightenSpeed);
		Solver.SetTotalRestLength(Next);
		if (FMath::IsNearlyEqual(Next, TargetLength, 0.01f))
		{
			TargetLength = -1.f;
		}
	}

	TArray<FRopeShape, TInlineAllocator<8>> Shapes;
	for (const FRopeCollider& Collider : Colliders)
	{
		FRopeShape Shape;
		if (ResolveCollider(Collider, Shape))
		{
			Shapes.Add(Shape);
		}
	}

	FRopeSolverParams Params;
	Params.Iterations = Iterations;
	Params.Damping = Damping;
	Params.Stiffness = Stiffness;
	Params.Friction = Friction;
	Params.NodeRadius = Thickness;
	Params.bSelfCollision = bSelfCollision;
	Params.Gravity = FVector(0.f, 0.f, World->GetGravityZ() * GravityScale);

	FRopeSolver::FWorldSweep Sweep;
	if (bCollideWithWorld)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(Rope), false);
		if (bIgnoreOwner)
		{
			QueryParams.AddIgnoredActor(GetOwner());
		}
		// Les acteurs qui tiennent la corde ne la repoussent pas (leurs membres : utiliser des Colliders)
		if (StartAnchor.Actor) { QueryParams.AddIgnoredActor(StartAnchor.Actor); }
		if (EndAnchor.Actor)   { QueryParams.AddIgnoredActor(EndAnchor.Actor); }

		const FCollisionShape Sphere = FCollisionShape::MakeSphere(Thickness);
		const ECollisionChannel Channel = CollisionChannel;
		Sweep = [World, QueryParams, Sphere, Channel](const FVector& From, const FVector& To, FVector& OutPosition, FVector& OutNormal)
		{
			FHitResult Hit;
			if (!World->SweepSingleByChannel(Hit, From, To, FQuat::Identity, Channel, Sphere, QueryParams))
			{
				return false;
			}
			OutNormal = Hit.ImpactNormal;
			OutPosition = Hit.bStartPenetrating
				? To + Hit.Normal * (Hit.PenetrationDepth + 0.1f)
				: Hit.Location + Hit.Normal * 0.1f;
			return true;
		};
	}

	const float SubDt = FrameDt / SubSteps;
	for (int32 Step = 0; Step < SubSteps; ++Step)
	{
		ApplyPins();
		Solver.Step(SubDt, Params, Shapes, bCollideWithWorld ? &Sweep : nullptr);
	}

	UpdateTube();
}

// ---------------------------------------------------------------- Tube

void URopeComponent::BuildTubeTopology()
{
	const int32 Rings = Solver.GetNumNodes();
	const int32 S = Sides;

	FDynamicMesh3 Mesh;
	Mesh.EnableAttributes();
	FDynamicMeshNormalOverlay* Normals = Mesh.Attributes()->PrimaryNormals();
	FDynamicMeshUVOverlay* UVs = Mesh.Attributes()->PrimaryUV();

	// Sommets et normales : un par (anneau, côté), mêmes indices. Positions remplies par UpdateTube.
	for (int32 I = 0; I < Rings * S; ++I)
	{
		Mesh.AppendVertex(FVector3d::ZeroVector);
		Normals->AppendElement(FVector3f::UnitZ());
	}

	// UV : S + 1 colonnes (couture), V proportionnel à la longueur (la texture ne s'étire pas)
	const float SegmentLength = Solver.GetTotalRestLength() / FMath::Max(1, Solver.GetNumSegments());
	const float VScale = SegmentLength / (2.f * PI * Thickness);
	for (int32 R = 0; R < Rings; ++R)
	{
		for (int32 Col = 0; Col <= S; ++Col)
		{
			UVs->AppendElement(FVector2f(float(Col) / S, R * VScale));
		}
	}

	// Enroulement : normale sortante dans la convention d'Unreal (même règle que PatchTerrain)
	for (int32 R = 0; R + 1 < Rings; ++R)
	{
		for (int32 Col = 0; Col < S; ++Col)
		{
			const int32 V00 = R * S + Col;
			const int32 V01 = R * S + (Col + 1) % S;
			const int32 V10 = (R + 1) * S + Col;
			const int32 V11 = (R + 1) * S + (Col + 1) % S;
			const int32 U00 = R * (S + 1) + Col;
			const int32 U01 = U00 + 1;
			const int32 U10 = (R + 1) * (S + 1) + Col;
			const int32 U11 = U10 + 1;

			const int32 T1 = Mesh.AppendTriangle(V00, V10, V11);
			if (T1 >= 0)
			{
				Normals->SetTriangle(T1, FIndex3i(V00, V10, V11));
				UVs->SetTriangle(T1, FIndex3i(U00, U10, U11));
			}
			const int32 T2 = Mesh.AppendTriangle(V00, V11, V01);
			if (T2 >= 0)
			{
				Normals->SetTriangle(T2, FIndex3i(V00, V11, V01));
				UVs->SetTriangle(T2, FIndex3i(U00, U11, U01));
			}
		}
	}

	BuiltRings = Rings;
	BuiltSides = S;
	SetMesh(MoveTemp(Mesh));
}

void URopeComponent::UpdateTube()
{
	if (!bGenerateMesh)
	{
		if (BuiltRings != 0)
		{
			SetMesh(FDynamicMesh3());
			BuiltRings = 0;
		}
		return;
	}

	const TArray<FVector>& Nodes = Solver.GetPositions();
	const int32 Rings = Nodes.Num();
	if (Rings < 2)
	{
		return;
	}

	const bool bNewTopology = BuiltRings != Rings || BuiltSides != Sides;
	if (bNewTopology)
	{
		BuildTubeTopology();
	}

	FDynamicMesh3* Mesh = GetMesh();
	if (!Mesh || !Mesh->HasAttributes())
	{
		return;
	}
	FDynamicMeshNormalOverlay* Normals = Mesh->Attributes()->PrimaryNormals();
	const FTransform& ToWorld = GetComponentTransform();
	const int32 S = Sides;

	// Repère par transport parallèle : l'anneau ne vrille pas d'une frame à l'autre
	FVector PrevTangent = (Nodes[1] - Nodes[0]).GetSafeNormal();
	FVector Normal, Binormal;
	PrevTangent.FindBestAxisVectors(Normal, Binormal);

	for (int32 R = 0; R < Rings; ++R)
	{
		const FVector& A = Nodes[FMath::Max(R - 1, 0)];
		const FVector& B = Nodes[FMath::Min(R + 1, Rings - 1)];
		FVector Tangent = (B - A).GetSafeNormal();
		if (Tangent.IsNearlyZero())
		{
			Tangent = PrevTangent;
		}
		Normal = (Normal - Tangent * FVector::DotProduct(Normal, Tangent)).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Tangent.FindBestAxisVectors(Normal, Binormal);
		}
		Binormal = Tangent ^ Normal;
		PrevTangent = Tangent;

		for (int32 Col = 0; Col < S; ++Col)
		{
			const float Angle = 2.f * PI * Col / S;
			const FVector Dir = Normal * FMath::Cos(Angle) + Binormal * FMath::Sin(Angle);
			const int32 Vid = R * S + Col;
			Mesh->SetVertex(Vid, ToWorld.InverseTransformPosition(Nodes[R] + Dir * Thickness));
			Normals->SetElement(Vid, FVector3f(ToWorld.InverseTransformVectorNoScale(Dir)));
		}
	}

	if (bNewTopology)
	{
		NotifyMeshUpdated();
	}
	else
	{
		FastNotifyPositionsUpdated(true, false, false);
	}
}
