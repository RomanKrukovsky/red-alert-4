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
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "RedAlert4" });

		bUseLoggingInShipping = true;
	}
}
