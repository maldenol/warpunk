using UnrealBuildTool;

public class Warpunk : ModuleRules
{
	public Warpunk(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "AIModule", "Core", "CoreUObject", "Engine", "EnhancedInput", "InputCore", "NavigationSystem", "UMG" });

		if (Target.bBuildEditor)
		{
			OptimizeCode = CodeOptimization.Never;
		}
	}
}
