#include "DwamLightSubsystem.h"

#include "DwamLightComponent.h"
#include "DwamLightTypes.h"
#include "DwamLightViewExtension.h"
#include "DwamLightingSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "RenderingThread.h"
#include "SceneViewExtension.h"
#include "TextureResource.h"
#include "UObject/UObjectIterator.h"

void UDwamLightSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Pas de rendu (tests en -nullrhi, commandlets) : la collecte reste utilisable, rien n'est envoyé
	if (FApp::CanEverRender())
	{
		ViewExtension = FSceneViewExtensions::NewExtension<FDwamLightViewExtension>(GetWorld());
		ResolveRenderTarget();
		// Envoi immédiat, sans lumière : un monde qui ne tick jamais (aperçu de matériau) écrit quand même un en-tête vide
		PushToRenderThread(TArray<FDwamLightData>());
	}

	// Les composants inscrits avant la création du subsystem
	UWorld* World = GetWorld();
	for (TObjectIterator<UDwamLightComponent> It; It; ++It)
	{
		if (It->GetWorld() == World && It->IsRegistered())
		{
			RegisterLight(*It);
		}
	}
}

void UDwamLightSubsystem::Deinitialize()
{
	if (ViewExtension.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(DwamLightsRelease)(
			[Extension = ViewExtension](FRHICommandListImmediate&)
			{
				Extension->SetFrameData_RenderThread(TArray<FDwamLightData>(), nullptr, 0);
			});
		ViewExtension.Reset();
	}
	Lights.Reset();
	RenderTarget = nullptr;

	Super::Deinitialize();
}

bool UDwamLightSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::Editor
		|| WorldType == EWorldType::EditorPreview
		|| WorldType == EWorldType::GamePreview;
}

TStatId UDwamLightSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDwamLightSubsystem, STATGROUP_Tickables);
}

void UDwamLightSubsystem::RegisterLight(UDwamLightComponent* Light)
{
	if (Light)
	{
		Lights.AddUnique(Light);
	}
}

void UDwamLightSubsystem::UnregisterLight(UDwamLightComponent* Light)
{
	Lights.RemoveSingleSwap(Light);
}

void UDwamLightSubsystem::ResolveRenderTarget()
{
	const UDwamLightingSettings* Settings = GetDefault<UDwamLightingSettings>();
	const FSoftObjectPath Path = Settings->LightsRenderTarget.ToSoftObjectPath();

	// Asset pas encore créé : ne pas tenter de le charger (évite un avertissement du moteur à chaque essai)
	if (Path.IsNull() || !FPackageName::DoesPackageExist(Path.GetLongPackageName()))
	{
		static bool bWarned = false;
		if (!bWarned)
		{
			bWarned = true;
			UE_LOG(LogDwamLighting, Warning,
				TEXT("Render target des lumières introuvable (%s). Le créer dans l'éditeur (RTF RGBA32f, 256 x 1) ou corriger Project Settings > Plugins > DWAM Lighting. Voir Docs/Guide_Lights.md."),
				*Path.ToString());
		}
		return;
	}

	RenderTarget = Settings->LightsRenderTarget.LoadSynchronous();
}

void UDwamLightSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<FDwamLightData> Data;
	Data.Reserve(Lights.Num());
	for (int32 Index = Lights.Num() - 1; Index >= 0; --Index)
	{
		const UDwamLightComponent* Light = Lights[Index].Get();
		if (!Light)
		{
			Lights.RemoveAtSwap(Index);
			continue;
		}

		FDwamLightData LightData;
		if (Light->BuildLightData(LightData))
		{
			Data.Add(LightData);
		}
	}
	NumActiveLights = Data.Num();

	if (!ViewExtension.IsValid())
	{
		return;
	}

	if (!RenderTarget)
	{
		ResolveCooldown -= DeltaTime;
		if (ResolveCooldown <= 0.f)
		{
			ResolveCooldown = 2.f;
			ResolveRenderTarget();
		}
	}

	PushToRenderThread(MoveTemp(Data));
}

void UDwamLightSubsystem::PushToRenderThread(TArray<FDwamLightData>&& Data)
{
	FTextureRenderTargetResource* Resource = RenderTarget ? RenderTarget->GameThread_GetRenderTargetResource() : nullptr;
	const int32 MaxLights = GetDefault<UDwamLightingSettings>()->MaxLights;

	ENQUEUE_RENDER_COMMAND(DwamLightsUpdate)(
		[Extension = ViewExtension, Data = MoveTemp(Data), Resource, MaxLights](FRHICommandListImmediate&) mutable
		{
			// La référence RHI est prise ici : si l'asset est recréé d'ici le prochain envoi, l'ancienne texture reste vivante
			FTextureRHIRef Texture = Resource ? Resource->GetRenderTargetTexture() : FTextureRHIRef();
			Extension->SetFrameData_RenderThread(MoveTemp(Data), Texture, MaxLights);
		});
}
