// Copyright (c) Red Alert 4 project.
//
// Headless authoritative server. It links the simulation modules but nothing that
// needs a renderer, so it can run in a container with no GPU.
using UnrealBuildTool;

public class RedAlert4ServerTarget : TargetRules
{
	public RedAlert4ServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.AddRange(new string[] { "RedAlert4" });

		bUsesSteam = false;
		bUseLoggingInShipping = true;
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