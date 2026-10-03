#include "GPUTraceSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GPUTraceSettings.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGPUTrace, Log, All);

namespace GPUTraceLocal
{
	static TAutoConsoleVariable<int32> CVarDebug(
		TEXT("GPUTrace.Debug"),
		0,
		TEXT("1 = dessine toutes les traces GPUTrace (vert = libre, rouge = impact, bleu = normale)."),
		ECVF_Cheat);

	/** Les identifiants voyagent en float côté GPU : exacts jusqu'à 2^24. */
	constexpr int32 MaxRequestId = 1 << 24;

	// Paramètres utilisateur attendus dans le système Niagara (sans le préfixe « User. »)
	static const FName ParamStarts(TEXT("GPUTrace_Starts"));
	static const FName ParamEnds(TEXT("GPUTrace_Ends"));
	static const FName ParamIds(TEXT("GPUTrace_Ids"));
	static const FName ParamSlots(TEXT("GPUTrace_Slots"));
	static const FName ParamCallback(TEXT("GPUTrace_Callback"));
}

// ============================================================================ Export handler

void UGPUTraceExportHandler::ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* NiagaraSystem, const FVector& SimulationPositionOffset)
{
	if (UGPUTraceSubsystem* Subsystem = Owner.Get())
	{
		Subsystem->HandleNiagaraResults(Data, SimulationPositionOffset);
	}
}

// ============================================================================ Cycle de vie

void UGPUTraceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Backend = GetDefault<UGPUTraceSettings>()->DefaultBackend;
	CPUTraceDelegate.BindUObject(this, &UGPUTraceSubsystem::OnCPUTraceDone);
}

void UGPUTraceSubsystem::Deinitialize()
{
	if (NiagaraComponent)
	{
		NiagaraComponent->DestroyComponent();
		NiagaraComponent = nullptr;
	}
	Pending.Reset();
	NiagaraQueue.Reset();
	Super::Deinitialize();
}

bool UGPUTraceSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UGPUTraceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGPUTraceSubsystem, STATGROUP_Tickables);
}

void UGPUTraceSubsystem::Tick(float DeltaTime)
{
	if (NiagaraQueue.Num() > 0 || bSlotsDirty)
	{
		if (EnsureNiagara())
		{
			DispatchNiagara();
		}
		else
		{
			// Pas de système Niagara : on repasse les requêtes en attente sur le CPU
			TArray<int32> Queue = MoveTemp(NiagaraQueue);
			for (const int32 Id : Queue)
			{
				if (FPendingTrace* Trace = Pending.Find(Id))
				{
					Trace->Backend = EGPUTraceBackend::CPU;
					DispatchCPU(*Trace);
				}
			}
		}
	}
	CheckNiagaraTimeouts();
}

// ============================================================================ API

void UGPUTraceSubsystem::SetBackend(EGPUTraceBackend NewBackend)
{
	Backend = NewBackend;
}

EGPUTraceBackend UGPUTraceSubsystem::GetActiveBackend() const
{
	if (Backend == EGPUTraceBackend::Niagara && (NiagaraComponent || !GetDefault<UGPUTraceSettings>()->NiagaraSystem.IsNull()))
	{
		return EGPUTraceBackend::Niagara;
	}
	return EGPUTraceBackend::CPU;
}

int32 UGPUTraceSubsystem::RequestTrace(FVector Start, FVector End, FGPUTraceResultDelegate OnResult, bool bDrawDebug)
{
	return RequestTraceNative(Start, End, [OnResult](const FGPUTraceResult& Result)
	{
		OnResult.ExecuteIfBound(Result);
	}, bDrawDebug);
}

int32 UGPUTraceSubsystem::RequestTraceNative(const FVector& Start, const FVector& End, TFunction<void(const FGPUTraceResult&)> Callback,
	bool bDrawDebug, const AActor* IgnoredActor)
{
	if (NextId >= GPUTraceLocal::MaxRequestId)
	{
		// Très rare : on repart à 1. Le GPU ne relance une trace que si l'identifiant d'un slot augmente,
		// donc on réinitialise aussi les slots et le système.
		NextId = 1;
		for (int32& Id : SlotIds)
		{
			Id = 0;
		}
		bSlotsDirty = true;
		if (NiagaraComponent)
		{
			NiagaraComponent->ResetSystem();
		}
	}

	const int32 Id = NextId++;

	FPendingTrace& Trace = Pending.Add(Id);
	Trace.Id = Id;
	Trace.Start = Start;
	Trace.End = End;
	Trace.Callback = MoveTemp(Callback);
	Trace.bDrawDebug = bDrawDebug;
	Trace.RequestFrame = GFrameCounter;
	Trace.IgnoredActor = IgnoredActor;
	Trace.Backend = GetActiveBackend();

	if (Trace.Backend == EGPUTraceBackend::CPU)
	{
		DispatchCPU(Trace);
	}
	else
	{
		NiagaraQueue.Add(Id);
	}
	return Id;
}

// ============================================================================ CPU

void UGPUTraceSubsystem::DispatchCPU(FPendingTrace& Trace)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const UGPUTraceSettings* Settings = GetDefault<UGPUTraceSettings>();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GPUTraceCPU), Settings->bCPUTraceComplex);
	if (const AActor* Ignored = Trace.IgnoredActor.Get())
	{
		Params.AddIgnoredActor(Ignored);
	}

	// Résultat livré à la frame suivante, sur le thread de jeu, via OnCPUTraceDone
	World->AsyncLineTraceByChannel(EAsyncTraceType::Single, Trace.Start, Trace.End, Settings->CPUTraceChannel.GetValue(),
		Params, FCollisionResponseParams::DefaultResponseParam, &CPUTraceDelegate, static_cast<uint32>(Trace.Id));
}

void UGPUTraceSubsystem::OnCPUTraceDone(const FTraceHandle& Handle, FTraceDatum& Datum)
{
	const int32 Id = static_cast<int32>(Datum.UserData);

	FGPUTraceResult Result;
	Result.Backend = EGPUTraceBackend::CPU;
	if (Datum.OutHits.Num() > 0 && Datum.OutHits[0].bBlockingHit)
	{
		const FHitResult& Hit = Datum.OutHits[0];
		Result.bHit = true;
		Result.Location = Hit.ImpactPoint;
		Result.Normal = Hit.ImpactNormal;
		Result.HitActor = Hit.GetActor();
	}
	Complete(Id, Result);
}

// ============================================================================ GPU (Niagara)

bool UGPUTraceSubsystem::EnsureNiagara()
{
	if (NiagaraComponent)
	{
		return true;
	}

	const UGPUTraceSettings* Settings = GetDefault<UGPUTraceSettings>();
	UNiagaraSystem* System = Settings->NiagaraSystem.LoadSynchronous();
	if (!System)
	{
		if (!bNiagaraUnavailableLogged)
		{
			UE_LOG(LogGPUTrace, Warning, TEXT("GPUTrace : aucun système Niagara configuré (Project Settings > Plugins > GPU Trace). Repli sur le CPU."));
			bNiagaraUnavailableLogged = true;
		}
		return false;
	}

	const int32 Slots = FMath::Clamp(Settings->NiagaraSlots, 1, 1000);
	SlotRequest.Init(INDEX_NONE, Slots);
	SlotIds.Init(0, Slots);
	SlotStarts.Init(FVector::ZeroVector, Slots);
	SlotEnds.Init(FVector::ZeroVector, Slots);
	bSlotsDirty = true;

	ExportHandler = NewObject<UGPUTraceExportHandler>(this);
	ExportHandler->Owner = this;

	NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, FVector::ZeroVector, FRotator::ZeroRotator,
		FVector(1.f), /*bAutoDestroy*/ false, /*bAutoActivate*/ false, ENCPoolMethod::None, /*bPreCullCheck*/ false);
	if (!NiagaraComponent)
	{
		UE_LOG(LogGPUTrace, Warning, TEXT("GPUTrace : impossible de créer le système Niagara %s. Repli sur le CPU."), *GetNameSafe(System));
		return false;
	}

	NiagaraComponent->SetVariableObject(GPUTraceLocal::ParamCallback, ExportHandler);
	NiagaraComponent->SetVariableInt(GPUTraceLocal::ParamSlots, Slots);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraComponent, GPUTraceLocal::ParamStarts, SlotStarts);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraComponent, GPUTraceLocal::ParamEnds, SlotEnds);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(NiagaraComponent, GPUTraceLocal::ParamIds, SlotIds);
	NiagaraComponent->Activate(true);

	UE_LOG(LogGPUTrace, Log, TEXT("GPUTrace : système Niagara %s actif (%d slots)."), *GetNameSafe(System), Slots);
	return true;
}

void UGPUTraceSubsystem::DispatchNiagara()
{
	// Attribue les requêtes en attente aux slots libres
	int32 SearchFrom = 0;
	while (NiagaraQueue.Num() > 0)
	{
		int32 FreeSlot = INDEX_NONE;
		for (int32 Slot = SearchFrom; Slot < SlotRequest.Num(); ++Slot)
		{
			if (SlotRequest[Slot] == INDEX_NONE)
			{
				FreeSlot = Slot;
				break;
			}
		}
		if (FreeSlot == INDEX_NONE)
		{
			break;   // tous les slots sont occupés : on réessaiera à la prochaine frame
		}
		SearchFrom = FreeSlot + 1;

		const int32 Id = NiagaraQueue[0];
		NiagaraQueue.RemoveAt(0, EAllowShrinking::No);

		FPendingTrace* Trace = Pending.Find(Id);
		if (!Trace)
		{
			continue;
		}
		Trace->Slot = FreeSlot;
		Trace->RequestFrame = GFrameCounter;
		SlotRequest[FreeSlot] = Id;
		SlotIds[FreeSlot] = Id;
		SlotStarts[FreeSlot] = Trace->Start;
		SlotEnds[FreeSlot] = Trace->End;
		bSlotsDirty = true;
	}

	if (bSlotsDirty && NiagaraComponent)
	{
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraComponent, GPUTraceLocal::ParamStarts, SlotStarts);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraComponent, GPUTraceLocal::ParamEnds, SlotEnds);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(NiagaraComponent, GPUTraceLocal::ParamIds, SlotIds);
		bSlotsDirty = false;
	}
}

void UGPUTraceSubsystem::HandleNiagaraResults(const TArray<FBasicParticleData>& Data, const FVector& PositionOffset)
{
	for (const FBasicParticleData& Item : Data)
	{
		// Encodage du système : Size = identifiant de requête, Position = impact, Velocity = normale (zéro si raté)
		const int32 Id = FMath::RoundToInt32(Item.Size);
		const FPendingTrace* Trace = Pending.Find(Id);
		if (!Trace)
		{
			continue;   // résultat périmé (requête déjà expirée)
		}

		if (SlotRequest.IsValidIndex(Trace->Slot) && SlotRequest[Trace->Slot] == Id)
		{
			SlotRequest[Trace->Slot] = INDEX_NONE;
		}

		FGPUTraceResult Result;
		Result.Backend = EGPUTraceBackend::Niagara;
		Result.bHit = Item.Velocity.SizeSquared() > 0.25;
		if (Result.bHit)
		{
			Result.Location = Item.Position + PositionOffset;
			Result.Normal = Item.Velocity.GetSafeNormal();
		}
		Complete(Id, Result);
	}
}

void UGPUTraceSubsystem::CheckNiagaraTimeouts()
{
	if (Pending.Num() == 0)
	{
		return;
	}

	const int32 Timeout = FMath::Max(2, GetDefault<UGPUTraceSettings>()->NiagaraTimeoutFrames);
	TArray<int32> Expired;
	for (const TPair<int32, FPendingTrace>& Pair : Pending)
	{
		const FPendingTrace& Trace = Pair.Value;
		if (Trace.Backend == EGPUTraceBackend::Niagara && Trace.Slot != INDEX_NONE && GFrameCounter - Trace.RequestFrame > static_cast<uint64>(Timeout))
		{
			Expired.Add(Pair.Key);
		}
	}

	for (const int32 Id : Expired)
	{
		const FPendingTrace& Trace = Pending.FindChecked(Id);
		if (SlotRequest.IsValidIndex(Trace.Slot) && SlotRequest[Trace.Slot] == Id)
		{
			SlotRequest[Trace.Slot] = INDEX_NONE;
		}
		FGPUTraceResult Result;
		Result.Backend = EGPUTraceBackend::Niagara;
		Result.bTimedOut = true;
		Complete(Id, Result);
	}
}

// ============================================================================ Résultats

void UGPUTraceSubsystem::Complete(int32 Id, FGPUTraceResult& Result)
{
	FPendingTrace Trace;
	if (!Pending.RemoveAndCopyValue(Id, Trace))
	{
		return;
	}

	Result.RequestId = Id;
	Result.Start = Trace.Start;
	Result.End = Trace.End;
	Result.LatencyFrames = static_cast<int32>(GFrameCounter - Trace.RequestFrame);
	if (!Result.bHit)
	{
		Result.Location = Trace.End;
		Result.Normal = FVector::ZeroVector;
	}
	Result.Distance = static_cast<float>(FVector::Dist(Trace.Start, Result.Location));

	if (Trace.bDrawDebug || GPUTraceLocal::CVarDebug.GetValueOnGameThread() != 0)
	{
		DrawResult(Result);
	}

	// Le callback peut lancer d'autres traces : la requête est déjà retirée de la table
	if (Trace.Callback)
	{
		Trace.Callback(Result);
	}
}

void UGPUTraceSubsystem::DrawResult(const FGPUTraceResult& Result) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const float Duration = GetDefault<UGPUTraceSettings>()->DebugDrawDuration;

	DrawDebugLine(World, Result.Start, Result.Location, Result.bHit ? FColor::Red : FColor::Green, false, Duration, 0, 1.5f);
	if (Result.bHit)
	{
		DrawDebugPoint(World, Result.Location, 10.f, FColor::Yellow, false, Duration);
		DrawDebugLine(World, Result.Location, Result.Location + Result.Normal * 30.f, FColor::Blue, false, Duration, 0, 1.5f);
	}
	else if (Result.bTimedOut)
	{
		DrawDebugPoint(World, Result.End, 10.f, FColor::Magenta, false, Duration);
	}
}
