// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

// Player intent: camera, selection, contextual orders. Engine-free so the rules can
// be regression-tested headlessly; the PlayerController is a thin adapter over it.
public class RA4Input : ModuleRules
{
	public RA4Input(ReadOnlyTargetRules Target) : base(Target)
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
			"RA4Simulation"
		});
	}
}
