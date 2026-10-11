#include "DwamLightingSettings.h"

#include "Engine/TextureRenderTarget2D.h"

UDwamLightingSettings::UDwamLightingSettings()
{
	// Emplacement par défaut : le dossier Content du plugin
	LightsRenderTarget = FSoftObjectPath(TEXT("/DwamLighting/RT_DwamLights.RT_DwamLights"));
}
