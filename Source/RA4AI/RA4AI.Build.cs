// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4AI : ModuleRules
{
	public RA4AI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"RA4Core",
			"RA4Simulation"
		});
			
		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine"
		});
	}
}
