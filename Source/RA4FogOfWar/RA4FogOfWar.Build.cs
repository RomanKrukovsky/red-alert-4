// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4FogOfWar : ModuleRules
{
	public RA4FogOfWar(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"RA4Core"
		});

			
		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine"
		});
	}
}
