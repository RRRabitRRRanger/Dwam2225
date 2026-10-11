using UnrealBuildTool;

public class LayoutKit : ModuleRules
{
	public LayoutKit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"GeometryCore",
			"GeometryFramework",
			"GraphKit",
			"LOA"
		});
	}
}
