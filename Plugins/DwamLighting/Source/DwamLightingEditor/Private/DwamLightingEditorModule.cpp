#include "ComponentVisualizer.h"
#include "DwamLightComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "Modules/ModuleManager.h"
#include "SceneManagement.h"
#include "UnrealEdGlobals.h"

/** Gizmo d'une lumière maison sélectionnée : portée, cône ou rectangle émetteur. */
class FDwamLightComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override
	{
		const UDwamLightComponent* Light = Cast<UDwamLightComponent>(Component);
		if (!Light)
		{
			return;
		}

		// Sans l'échelle : la portée et les tailles sont en centimètres monde
		const FTransform Transform(Light->GetComponentQuat(), Light->GetComponentLocation());
		const FLinearColor RangeColor(0.6f, 1.f, 1.f);
		const FLinearColor InnerColor(0.3f, 0.5f, 0.5f);

		switch (Light->Type)
		{
		case EDwamLightType::Spot:
			DrawWireSphereCappedCone(PDI, Transform, Light->Range, Light->OuterConeAngle, 32, 8, 10, RangeColor, SDPG_World);
			if (Light->InnerConeAngle > UE_KINDA_SMALL_NUMBER && Light->InnerConeAngle < Light->OuterConeAngle)
			{
				DrawWireSphereCappedCone(PDI, Transform, Light->Range, Light->InnerConeAngle, 32, 8, 10, InnerColor, SDPG_World);
			}
			break;

		case EDwamLightType::Rect:
		{
			const FVector Center = Transform.GetLocation();
			const FVector Forward = Transform.GetUnitAxis(EAxis::X);
			const FVector Right = Transform.GetUnitAxis(EAxis::Y) * (Light->SourceWidth * 0.5f);
			const FVector Up = Transform.GetUnitAxis(EAxis::Z) * (Light->SourceHeight * 0.5f);
			const FVector Corners[4] = { Center - Right - Up, Center + Right - Up, Center + Right + Up, Center - Right + Up };
			for (int32 Index = 0; Index < 4; ++Index)
			{
				PDI->DrawLine(Corners[Index], Corners[(Index + 1) % 4], RangeColor, SDPG_World);
			}
			// Le côté qui émet
			PDI->DrawLine(Center, Center + Forward * FMath::Min(Light->Range, 100.f), RangeColor, SDPG_World);
			DrawWireSphereAutoSides(PDI, Transform, InnerColor, Light->Range, SDPG_World);
			break;
		}

		default:
			DrawWireSphereAutoSides(PDI, Transform, RangeColor, Light->Range, SDPG_World);
			break;
		}
	}
};

class FDwamLightingEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (GUnrealEd)
		{
			TSharedPtr<FComponentVisualizer> Visualizer = MakeShared<FDwamLightComponentVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(UDwamLightComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
	}

	virtual void ShutdownModule() override
	{
		if (GUnrealEd && UObjectInitialized())
		{
			GUnrealEd->UnregisterComponentVisualizer(UDwamLightComponent::StaticClass()->GetFName());
		}
	}
};

IMPLEMENT_MODULE(FDwamLightingEditorModule, DwamLightingEditor)
