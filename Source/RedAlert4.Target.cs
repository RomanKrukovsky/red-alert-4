// Copyright (c) Red Alert 4 project.
using UnrealBuildTool;

public class RedAlert4Target : TargetRules
{
	public RedAlert4Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "RedAlert4" });
		ExtraModuleNames.Add("RA4Navigation");
		ExtraModuleNames.Add("RA4FogOfWar");
		ExtraModuleNames.Add("RA4AI");
		ExtraModuleNames.Add("RA4Network");
		ExtraModuleNames.Add("RA4Campaign");
		ExtraModuleNames.Add("RA4UI");
	}
}
