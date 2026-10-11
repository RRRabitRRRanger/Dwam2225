#include "DwamLightViewExtension.h"

#include "PixelFormat.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "SceneView.h"

FDwamLightViewExtension::FDwamLightViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld)
	: FWorldSceneViewExtension(AutoRegister, InWorld)
{
}

void FDwamLightViewExtension::SetFrameData_RenderThread(TArray<FDwamLightData>&& InLights, FTextureRHIRef InTexture, int32 InMaxLights)
{
	check(IsInRenderingThread());
	Lights = MoveTemp(InLights);
	Texture = MoveTemp(InTexture);
	MaxLights = InMaxLights;
}

void FDwamLightViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	if (!Texture.IsValid() || InViewFamily.Views.Num() == 0 || InViewFamily.Views[0] == nullptr)
	{
		return;
	}

	const FRHITextureDesc& Desc = Texture->GetDesc();
	const int32 Width = Desc.Extent.X;
	if (Desc.Format != PF_A32B32G32R32F || DwamLightPacking::CapacityForWidth(Width) < 1)
	{
		if (!bWarnedFormat)
		{
			bWarnedFormat = true;
			UE_LOG(LogDwamLighting, Warning,
				TEXT("Le render target des lumières doit être en RTF RGBA32f et large d'au moins 4 pixels (actuel : %s, %d px). Rien n'est envoyé."),
				GPixelFormats[Desc.Format].Name, Width);
		}
		return;
	}

	// Une seule caméra par rendu : les positions sont relatives à la première vue (pas de split-screen)
	const FSceneView& View = *InViewFamily.Views[0];
	const FVector PreViewTranslation = View.ViewMatrices.GetPreViewTranslation();
	const FVector ViewOrigin = View.ViewMatrices.GetViewOrigin();

	const int32 Capacity = FMath::Min(MaxLights, DwamLightPacking::CapacityForWidth(Width));
	const int32 NumLights = FMath::Min(Lights.Num(), Capacity);

	// Dans l'ordre de collecte tant que tout tient ; sinon les plus importantes pour cette caméra d'abord
	TArray<int32, TInlineAllocator<128>> Order;
	Order.SetNumUninitialized(Lights.Num());
	for (int32 Index = 0; Index < Lights.Num(); ++Index)
	{
		Order[Index] = Index;
	}

	if (Lights.Num() > Capacity)
	{
		if (!bWarnedBudget)
		{
			bWarnedBudget = true;
			UE_LOG(LogDwamLighting, Warning,
				TEXT("%d lumières pour un budget de %d : seules les plus importantes pour la caméra sont gardées (taille à l'écran x luminosité)."),
				Lights.Num(), Capacity);
		}

		TArray<float, TInlineAllocator<128>> Importance;
		Importance.SetNumUninitialized(Lights.Num());
		for (int32 Index = 0; Index < Lights.Num(); ++Index)
		{
			const FDwamLightData& Light = Lights[Index];
			const float Distance = (float)FVector::Dist(Light.Position, ViewOrigin);
			Importance[Index] = Light.Range / FMath::Max(Distance, Light.Range * 0.1f) * Light.Color.GetMax();
		}
		Order.Sort([&Importance](int32 A, int32 B) { return Importance[A] > Importance[B]; });
	}
	else
	{
		bWarnedBudget = false;
	}

	TArray<uint32> Words;
	Words.SetNumZeroed(Width * DwamLightPacking::ChannelsPerPixel);
	DwamLightPacking::PackHeader(NumLights, Words.GetData());
	for (int32 Slot = 0; Slot < NumLights; ++Slot)
	{
		const int32 FirstPixel = DwamLightPacking::HeaderPixels + Slot * DwamLightPacking::PixelsPerLight;
		DwamLightPacking::PackLight(Lights[Order[Slot]], PreViewTranslation, &Words[FirstPixel * DwamLightPacking::ChannelsPerPixel]);
	}

	// Passe sans ressource RDG : la texture reste dans son état de lecture pour les matériaux,
	// le RHI gère lui-même l'aller-retour vers l'état de copie.
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("DwamLights.Upload (%d)", NumLights),
		ERDGPassFlags::NeverCull,
		[TextureRHI = Texture, Words = MoveTemp(Words), Width](FRHICommandListImmediate& RHICmdList)
		{
			const FUpdateTextureRegion2D Region(0, 0, 0, 0, (uint32)Width, 1);
			const uint32 Pitch = (uint32)Width * DwamLightPacking::ChannelsPerPixel * sizeof(uint32);
			RHICmdList.UpdateTexture2D(TextureRHI, 0, Region, Pitch, (const uint8*)Words.GetData());
		});
}
