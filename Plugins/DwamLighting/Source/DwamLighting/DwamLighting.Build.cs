using UnrealBuildTool;

public class DwamLighting : ModuleRules
{
	public DwamLighting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Chargé en PostConfigInit (avant la compilation des shaders) : dépendances minimales
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"Projects",
			"RenderCore"
		});
	}
}
