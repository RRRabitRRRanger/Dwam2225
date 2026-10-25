using UnrealBuildTool;

public class SDFEyesRuntime : ModuleRules
{
	public SDFEyesRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Classes de jeu (forme d'œil, component) : module séparé, chargé normalement.
		// Le module SDFEyes reste en PostConfigInit pour le chemin des shaders.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}
