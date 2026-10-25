using UnrealBuildTool;

public class CharacterKit : ModuleRules
{
	public CharacterKit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SDFEyesRuntime"
		});

		// Pour les tests fonctionnels (captures)
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ImageCore",
			"RenderCore"
		});
	}
}
