using UnrealBuildTool;

public class DwamLightingRuntime : ModuleRules
{
	public DwamLightingRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Module de jeu du pipeline d'éclairage (composants). Chargé en phase Default :
		// le module « DwamLighting » (PostConfigInit) reste réservé au mapping des shaders.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings"
		});

		// Envoi des lumières au GPU (Scene View Extension, RDG)
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderCore",
			"RHI"
		});
	}
}
