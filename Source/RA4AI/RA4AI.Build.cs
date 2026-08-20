// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

// The computer opponent. Engine-free: it reads SimWorld read-only and emits the same
// Command objects a human produces, so it passes the same server validation and can
// be played against itself headlessly in a test.
public class RA4AI : ModuleRules
{
	public RA4AI(ReadOnlyTargetRules Target) : base(Target)
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
			"RA4Recon",
			"RA4FogOfWar"
		});

	}
}
