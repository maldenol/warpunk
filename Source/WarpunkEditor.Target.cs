using UnrealBuildTool;

public class WarpunkEditorTarget : TargetRules
{
	public WarpunkEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "Warpunk" } );
	}
}
