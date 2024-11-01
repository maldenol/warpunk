using UnrealBuildTool;

public class WarpunkTarget : TargetRules
{
	public WarpunkTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "Warpunk" } );
	}
}
