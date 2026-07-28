// Copyright (c) Red Alert 4 project.
using UnrealBuildTool;

public class RedAlert4Target : TargetRules
{
	public RedAlert4Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.AddRange(new string[] { "RedAlert4" });
	}
}

		ExtraModuleNames.Add("RA4Units");
		ExtraModuleNames.Add("RA4Buildings");
		ExtraModuleNames.Add("RA4Economy");
		ExtraModuleNames.Add("RA4Combat");
		ExtraModuleNames.Add("RA4Navigation");
		ExtraModuleNames.Add("RA4FogOfWar");
		ExtraModuleNames.Add("RA4AI");
		ExtraModuleNames.Add("RA4Network");
		ExtraModuleNames.Add("RA4Campaign");
		ExtraModuleNames.Add("RA4SaveSystem");
		ExtraModuleNames.Add("RA4UI");
		ExtraModuleNames.Add("RA4Audio");
		ExtraModuleNames.Add("RA4Editor");
		ExtraModuleNames.Add("RA4Modding");
		ExtraModuleNames.Add("RA4Diagnostics");