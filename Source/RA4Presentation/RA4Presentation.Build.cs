// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

// Read-only projection of simulation state into shapes the UI can consume.
// Engine-free so the rules -- what counts as low power, what blocks a build card,
// how a mixed selection is summarised, when alerts merge -- are covered by headless
// tests instead of being verified by looking at a widget.
public class RA4Presentation : ModuleRules
{
	public RA4Presentation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;

		PublicIncludePaths.Add(ModuleDirectory + "/Public");
		PrivateIncludePaths.Add(ModuleDirectory + "/Private");

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"RA4Core",
			"RA4Content",
			"RA4Simulation",
			"RA4FogOfWar"
		});
	}
}
