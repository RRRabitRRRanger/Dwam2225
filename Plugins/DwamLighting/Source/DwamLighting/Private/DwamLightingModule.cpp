#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

/**
 * Module du pipeline d'éclairage maison.
 * Pour l'instant : rend les shaders du plugin accessibles sous le chemin virtuel /Plugin/DwamLighting,
 * utilisable dans les « Include File Paths » des nœuds Custom des matériaux.
 * Doit être chargé en PostConfigInit (avant la compilation des shaders).
 */
class FDwamLightingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("DwamLighting"));
		if (Plugin.IsValid())
		{
			const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
			AddShaderSourceDirectoryMapping(TEXT("/Plugin/DwamLighting"), ShaderDir);
		}
	}
};

IMPLEMENT_MODULE(FDwamLightingModule, DwamLighting)
