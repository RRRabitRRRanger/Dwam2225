#include "GPUTraceAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GPUTraceSubsystem.h"

UGPUTraceAsyncAction* UGPUTraceAsyncAction::AsyncGPUTrace(UObject* WorldContextObject, FVector Start, FVector End, bool bDrawDebug)
{
	UGPUTraceAsyncAction* Action = NewObject<UGPUTraceAsyncAction>();
	Action->WorldContext = WorldContextObject;
	Action->TraceStart = Start;
	Action->TraceEnd = End;
	Action->bDebug = bDrawDebug;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UGPUTraceAsyncAction::Activate()
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext.Get(), EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	UGPUTraceSubsystem* Subsystem = World ? World->GetSubsystem<UGPUTraceSubsystem>() : nullptr;
	if (!Subsystem)
	{
		// Monde sans sous-système (éditeur hors jeu) : on répond « raté » tout de suite
		FGPUTraceResult Result;
		Result.Start = TraceStart;
		Result.End = TraceEnd;
		Result.Location = TraceEnd;
		OnMiss.Broadcast(Result);
		SetReadyToDestroy();
		return;
	}

	TWeakObjectPtr<UGPUTraceAsyncAction> WeakThis(this);
	Subsystem->RequestTraceNative(TraceStart, TraceEnd, [WeakThis](const FGPUTraceResult& Result)
	{
		if (UGPUTraceAsyncAction* Self = WeakThis.Get())
		{
			if (Result.bHit)
			{
				Self->OnHit.Broadcast(Result);
			}
			else
			{
				Self->OnMiss.Broadcast(Result);
			}
			Self->SetReadyToDestroy();
		}
	}, bDebug);
}
