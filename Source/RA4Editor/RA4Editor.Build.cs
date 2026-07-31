// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4Editor : ModuleRules
{
	public RA4Editor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"RA4Core",
			"RA4Content",
			"RA4Simulation"
		});
			
		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"RedAlert4",
			"Json",
			"JsonUtilities",
			"GameplayTags",
			"Landscape",
			"AudioEditor",
			"MaterialEditor"
		});
	}
}
