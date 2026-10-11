using UnrealBuildTool;

public class DwamLightingEditor : ModuleRules
{
	public DwamLightingEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Outils d'éditeur du pipeline d'éclairage : gizmos des lumières maison
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"DwamLightingRuntime"
		});
	}
}
