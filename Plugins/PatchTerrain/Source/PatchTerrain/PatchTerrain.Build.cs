using UnrealBuildTool;

public class PatchTerrain : ModuleRules
{
	public PatchTerrain(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GeometryCore",
			"GeometryFramework"
		});

		// Uniquement pour regrouper les reconstructions en éditeur (timer de l'éditeur)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
