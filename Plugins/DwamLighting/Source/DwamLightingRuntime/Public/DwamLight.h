#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DwamLight.generated.h"

class UDwamLightComponent;

/** Lumière maison prête à poser dans un niveau (Place Actors > « Dwam Light »). */
UCLASS(meta = (DisplayName = "Dwam Light"))
class DWAMLIGHTINGRUNTIME_API ADwamLight : public AActor
{
	GENERATED_BODY()

public:
	ADwamLight();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dwam Light")
	TObjectPtr<UDwamLightComponent> LightComponent;
};
