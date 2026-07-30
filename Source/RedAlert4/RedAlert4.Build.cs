// Copyright (c) Red Alert 4 project.
using UnrealBuildTool;

// The presentation and integration module: everything that touches Actors, input,
// UI and audio lives here and talks to the simulation through commands and events.
// Dependencies point one way only -- the simulation never includes this module.
public class RedAlert4 : ModuleRules
{
	public RedAlert4(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"RA4Core",
			"RA4Content",
			"RA4Input",
			"RA4Simulation",
			"RA4Replay",
			"RA4FogOfWar",
		});


		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UMG",
			"RA4UI",
			"RA4Presentation",
			"CommonUI",
			"ModelViewViewModel",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"NetCore",
			"Projects",
		});
	}
}
