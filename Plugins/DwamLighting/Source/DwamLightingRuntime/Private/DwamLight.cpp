#include "DwamLight.h"

#include "DwamLightComponent.h"

ADwamLight::ADwamLight()
{
	PrimaryActorTick.bCanEverTick = false;

	LightComponent = CreateDefaultSubobject<UDwamLightComponent>(TEXT("LightComponent"));
	RootComponent = LightComponent;
}
