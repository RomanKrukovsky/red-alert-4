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
			"RA4Simulation"
		});
	}
}
