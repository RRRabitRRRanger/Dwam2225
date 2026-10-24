#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

/**
 * Module des paupières SDF.
 * Pour l'instant : rend les shaders du plugin accessibles sous le chemin virtuel /Plugin/SDFEyes,
 * utilisable dans les « Include File Paths » des nœuds Custom des matériaux.
 * Doit être chargé en PostConfigInit (avant la compilation des shaders).
 */
class FSDFEyesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SDFEyes"));
		if (Plugin.IsValid())
		{
			const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
			AddShaderSourceDirectoryMapping(TEXT("/Plugin/SDFEyes"), ShaderDir);
		}
	}
};

IMPLEMENT_MODULE(FSDFEyesModule, SDFEyes)
