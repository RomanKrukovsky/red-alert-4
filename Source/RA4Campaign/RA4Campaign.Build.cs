// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4Campaign : ModuleRules
{
	public RA4Campaign(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"RA4Core",
			// MissionRuntime resolves each spawn's ContentId against the database
			// before placing it, so a mission naming content this build does not
			// have is skipped rather than crashing.
			"RA4Content",
			"RA4Simulation"
		});
	}
}
