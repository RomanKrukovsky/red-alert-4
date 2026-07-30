// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4Combat : ModuleRules
{
	public RA4Combat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"RA4Core",
			"RA4Simulation"
		});
	}
}
