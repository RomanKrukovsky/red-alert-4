// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4UI : ModuleRules
{
	public RA4UI(ReadOnlyTargetRules Target) : base(Target)
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
