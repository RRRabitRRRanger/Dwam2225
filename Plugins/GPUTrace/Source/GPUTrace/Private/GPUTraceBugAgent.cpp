#include "GPUTraceBugAgent.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GPUTraceSubsystem.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogGPUTraceBug, Log, All);

namespace GPUTraceBugLocal
{
	constexpr float SphereMeshRadius = 50.f;
	constexpr int32 GoalRay = 0;
	constexpr int32 WallRay = 1;
	constexpr int32 FirstCandidate = 2;
}

AGPUTraceBugAgent::AGPUTraceBugAgent()
{
	PrimaryActorTick.bCanEverTick = true;

	// La « boîte qui piège » : l'agent ne s'en sert pas pour se déplacer (pas de physique, pas de sweep)
	TrapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TrapBox"));
	TrapBox->SetBoxExtent(FVector(30.f));
	TrapBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = TrapBox;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(TrapBox);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Invisible pour le ray tracing : les rayons GPU partent du centre et ne doivent pas toucher le corps
	Body->SetVisibleInRayTracing(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		Body->SetStaticMesh(SphereFinder.Object);
	}
	Body->SetRelativeScale3D(FVector(AgentRadius / GPUTraceBugLocal::SphereMeshRadius));
}

void AGPUTraceBugAgent::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Body->SetRelativeScale3D(FVector(AgentRadius / GPUTraceBugLocal::SphereMeshRadius));
}

void AGPUTraceBugAgent::BeginPlay()
{
	Super::BeginPlay();
	StartTransform = GetActorTransform();
	if (!GetWorld()->GetSubsystem<UGPUTraceSubsystem>())
	{
		Say(TEXT("sous-système GPUTrace introuvable : rien ne peut marcher"), FColor::Red);
	}
	else if (!bAutoStart)
	{
		Say(TEXT("prêt (Auto Start décoché : utiliser Relancer)"), FColor::Silver);
	}
	if (bAutoStart)
	{
		StartBug();
	}
}

FVector AGPUTraceBugAgent::GetGoalLocation() const
{
	if (bHasGoalOverride)
	{
		return GoalOverride;
	}
	if (GoalActor)
	{
		return GoalActor->GetActorLocation();
	}
	// En éditeur, le but suit l'acteur ; en jeu, il reste fixe par rapport au point de départ
	const FTransform& Base = HasActorBegunPlay() ? StartTransform : GetActorTransform();
	return Base.TransformPosition(GoalOffset);
}

void AGPUTraceBugAgent::StartBug()
{
	Mode = EGPUTraceBugMode::Idle;   // pour que le message « départ » s'affiche à chaque relance
	++BatchSerial;           // ignore les résultats d'un éventuel lot précédent
	Batch.Reset();
	Cycles = 0;
	bTriedReverse = false;
	Trail.Reset();
	Trail.Add(GetActorLocation());
	SetMode(EGPUTraceBugMode::GoToGoal, TEXT("départ"));
}

void AGPUTraceBugAgent::StopBug()
{
	++BatchSerial;
	Batch.Reset();
	SetMode(EGPUTraceBugMode::Idle, TEXT("arrêt demandé"));
}

void AGPUTraceBugAgent::Say(const FString& Text, const FColor& Color) const
{
	UE_LOG(LogGPUTraceBug, Log, TEXT("%s : %s"), *GetName(), *Text);
	if (bPrintMessages && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, Color, FString::Printf(TEXT("[%s] %s"), *GetActorNameOrLabel(), *Text));
	}
}

void AGPUTraceBugAgent::SetMode(EGPUTraceBugMode NewMode, const FString& Reason)
{
	if (Mode == NewMode)
	{
		return;
	}
	Mode = NewMode;

	const FString ModeName = UEnum::GetDisplayValueAsText(NewMode).ToString();
	const FColor Color = NewMode == EGPUTraceBugMode::Reached ? FColor::Green
		: NewMode == EGPUTraceBugMode::Stuck ? FColor::Red
		: FColor::Cyan;
	Say(FString::Printf(TEXT("%s (%s)"), *ModeName, *Reason), Color);
	OnModeChanged.Broadcast(NewMode);
}

bool AGPUTraceBugAgent::IsFree(const FGPUTraceResult& Result, float Length)
{
	return !Result.bHit || Result.Distance >= Length * 0.98f;
}

void AGPUTraceBugAgent::MoveTo(const FVector& NewLocation)
{
	// Déplacement direct : la boîte de collision de l'agent ne compte pas, seules les traces décident
	SetActorLocation(NewLocation, /*bSweep*/ false);
	Trail.Add(NewLocation);
	if (Trail.Num() > 4000)
	{
		Trail.RemoveAt(0, 1000);
	}
}

void AGPUTraceBugAgent::Think()
{
	UGPUTraceSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGPUTraceSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	const FVector P = GetActorLocation();
	const FVector ToGoal = GetGoalLocation() - P;
	const float Dist = static_cast<float>(ToGoal.Size());
	if (Dist <= AgentRadius + StepSize * 0.5f)
	{
		SetMode(EGPUTraceBugMode::Reached, FString::Printf(TEXT("%d cycles"), Cycles));
		if (bWander)
		{
			PickWanderGoal();
		}
		return;
	}
	if (++Cycles > MaxCycles)
	{
		SetMode(EGPUTraceBugMode::Stuck, TEXT("trop de cycles"));
		if (bWander)
		{
			PickWanderGoal();
		}
		return;
	}

	const FVector GoalDir = ToGoal / Dist;
	// Le rayon dépasse le pas d'une marge de 1,5 rayon : après le pas, la paroi reste à plus d'un rayon
	const float Probe = StepSize + AgentRadius * 1.5f;

	TSharedPtr<FBatch> NewBatch = MakeShared<FBatch>();
	NewBatch->Serial = BatchSerial;

	// Rayon 0 : vers le but
	NewBatch->Directions.Add(GoalDir);
	NewBatch->Lengths.Add(FMath::Min(Dist, Probe));

	if (Mode == EGPUTraceBugMode::FollowWall)
	{
		const FVector N = WallNormal;
		const FVector F = FollowDir;
		const FVector S = FVector::CrossProduct(N, F).GetSafeNormal() * (bFollowRight ? 1.f : -1.f);

		// Rayon 1 : palpe la paroi
		NewBatch->Directions.Add(-N);
		NewBatch->Lengths.Add(AgentRadius * 3.f);

		// Candidats, par ordre de préférence : vers la paroi (contourner les angles), tout droit, puis en s'écartant
		auto Pitch = [&](float Deg)
		{
			const float R = FMath::DegreesToRadians(Deg);
			return (F * FMath::Cos(R) - N * FMath::Sin(R)).GetSafeNormal();
		};
		auto Yaw = [&](float Deg)
		{
			const float R = FMath::DegreesToRadians(Deg);
			return (F * FMath::Cos(R) + S * FMath::Sin(R)).GetSafeNormal();
		};
		const FVector Candidates[] =
		{
			Pitch(45.f), Pitch(0.f), Pitch(-30.f), Pitch(-60.f),
			Yaw(45.f), Yaw(-45.f), Pitch(-90.f), Yaw(90.f), Yaw(-90.f),
		};
		for (const FVector& Dir : Candidates)
		{
			NewBatch->Directions.Add(Dir);
			NewBatch->Lengths.Add(Probe);
		}
	}

	const int32 Count = NewBatch->Directions.Num();
	NewBatch->Results.SetNum(Count);
	NewBatch->Remaining = Count;
	Batch = NewBatch;
	BatchStartFrame = GFrameCounter;

	TWeakObjectPtr<AGPUTraceBugAgent> WeakThis(this);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector End = P + NewBatch->Directions[Index] * NewBatch->Lengths[Index];
		Subsystem->RequestTraceNative(P, End, [WeakThis, NewBatch, Index](const FGPUTraceResult& Result)
		{
			NewBatch->Results[Index] = Result;
			if (--NewBatch->Remaining == 0)
			{
				AGPUTraceBugAgent* Self = WeakThis.Get();
				if (Self && Self->Batch == NewBatch && NewBatch->Serial == Self->BatchSerial)
				{
					Self->OnBatchDone();
				}
			}
		}, bDrawDebug, this);
	}
}

void AGPUTraceBugAgent::OnBatchDone()
{
	using namespace GPUTraceBugLocal;

	const TSharedPtr<FBatch> Done = Batch;
	Batch.Reset();
	if (!Done.IsValid())
	{
		return;
	}

	const FVector P = GetActorLocation();
	const FVector ToGoal = GetGoalLocation() - P;
	const float Dist = static_cast<float>(ToGoal.Size());
	const FVector GoalDir = ToGoal.GetSafeNormal();
	const FGPUTraceResult& GoalHit = Done->Results[GoalRay];
	const bool bGoalFree = IsFree(GoalHit, Done->Lengths[GoalRay]);

	if (Mode == EGPUTraceBugMode::GoToGoal)
	{
		if (bGoalFree)
		{
			MoveTo(P + GoalDir * FMath::Min(StepSize, Dist));
			return;
		}

		// Obstacle : on passe en suivi de paroi, en mémorisant la distance au but au point de contact
		WallNormal = GoalHit.Normal.IsNearlyZero() ? -GoalDir : GoalHit.Normal;
		HitDistToGoal = Dist;
		FVector Tangent = FVector::VectorPlaneProject(GoalDir, WallNormal);
		if (Tangent.SizeSquared() < 0.01)
		{
			// But droit dans le mur : on choisit un côté
			Tangent = FVector::CrossProduct(WallNormal, FVector::UpVector);
			if (Tangent.SizeSquared() < 0.01)
			{
				Tangent = FVector::CrossProduct(WallNormal, FVector::ForwardVector);
			}
			Tangent *= bFollowRight ? 1.f : -1.f;
		}
		FollowDir = Tangent.GetSafeNormal();
		bTriedReverse = false;
		SetMode(EGPUTraceBugMode::FollowWall, FString::Printf(TEXT("obstacle à %.0f cm du but"), Dist));
		return;
	}

	if (Mode != EGPUTraceBugMode::FollowWall)
	{
		return;
	}

	// Critère de sortie (Bug2) : voie libre vers le but ET plus proche qu'au point de contact
	if (bGoalFree && Dist < HitDistToGoal - StepSize * 0.5f)
	{
		SetMode(EGPUTraceBugMode::GoToGoal, FString::Printf(TEXT("voie libre, %.0f cm du but"), Dist));
		MoveTo(P + GoalDir * FMath::Min(StepSize, Dist));
		return;
	}

	// Mise à jour de la paroi
	const FGPUTraceResult& WallHit = Done->Results[WallRay];
	bWallSeen = WallHit.bHit;
	if (WallHit.bHit && !WallHit.Normal.IsNearlyZero())
	{
		WallNormal = WallHit.Normal;
		LastWallDistance = WallHit.Distance;
	}

	// Premier candidat libre
	for (int32 Index = FirstCandidate; Index < Done->Directions.Num(); ++Index)
	{
		if (!IsFree(Done->Results[Index], Done->Lengths[Index]))
		{
			continue;
		}

		const FVector Dir = Done->Directions[Index];
		FVector NewLocation = P + Dir * StepSize;

		// Garde ses distances avec la paroi
		if (bWallSeen && LastWallDistance < AgentRadius)
		{
			NewLocation += WallNormal * (AgentRadius - LastWallDistance) * 0.5f;
		}
		MoveTo(NewLocation);

		const FVector Tangent = FVector::VectorPlaneProject(Dir, WallNormal);
		FollowDir = Tangent.SizeSquared() > 0.01 ? Tangent.GetSafeNormal() : Dir;
		bTriedReverse = false;
		return;
	}

	// Aucune issue : on tente une fois l'autre sens, puis on abandonne
	if (!bTriedReverse)
	{
		FollowDir = -FollowDir;
		bTriedReverse = true;
		Say(TEXT("cul-de-sac, demi-tour"), FColor::Orange);
		return;
	}
	SetMode(EGPUTraceBugMode::Stuck, TEXT("aucune direction libre"));
	if (bWander)
	{
		PickWanderGoal();
	}
}

void AGPUTraceBugAgent::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bActive = Mode == EGPUTraceBugMode::GoToGoal || Mode == EGPUTraceBugMode::FollowWall;
	// Garde : un lot de traces sans réponse au bout de 2 s environ est abandonné et relancé
	if (bActive && Batch.IsValid() && GFrameCounter - BatchStartFrame > 120)
	{
		Say(TEXT("traces sans réponse, on relance le lot"), FColor::Orange);
		++BatchSerial;
		Batch.Reset();
	}

	if (bActive && !Batch.IsValid())
	{
		Think();
	}

	if (bDrawDebug)
	{
		const UWorld* World = GetWorld();
		for (int32 Index = 1; Index < Trail.Num(); ++Index)
		{
			DrawDebugLine(World, Trail[Index - 1], Trail[Index], FColor::Emerald, false, 0.f, 0, 2.f);
		}
		DrawDebugSphere(World, GetGoalLocation(), AgentRadius, 12, FColor::Yellow, false, 0.f, 0, 1.f);
		DrawDebugString(World, GetActorLocation() + FVector::UpVector * AgentRadius * 2.f,
			UEnum::GetDisplayValueAsText(Mode).ToString(), nullptr, FColor::White, 0.f, true);
		if (Mode == EGPUTraceBugMode::FollowWall)
		{
			DrawDebugDirectionalArrow(World, GetActorLocation(), GetActorLocation() + FollowDir * AgentRadius * 3.f, 8.f, FColor::Orange, false, 0.f, 0, 1.5f);
		}
	}
}

void AGPUTraceBugAgent::Restart()
{
	StartTransform = GetActorTransform();
	bHasGoalOverride = false;
	StartBug();
}

void AGPUTraceBugAgent::PickWanderGoal()
{
	// Nouveau but au hasard autour de la position actuelle, puis on repart
	const float Distance = FMath::FRandRange(0.3f, 1.f) * WanderRadius;
	GoalOverride = GetActorLocation() + FMath::VRand() * Distance;
	bHasGoalOverride = true;
	Say(FString::Printf(TEXT("errance : nouveau but à %.0f cm"), Distance), FColor::Silver);

	++BatchSerial;
	Batch.Reset();
	Cycles = 0;
	bTriedReverse = false;
	Mode = EGPUTraceBugMode::Idle;
	SetMode(EGPUTraceBugMode::GoToGoal, TEXT("errance"));
}
