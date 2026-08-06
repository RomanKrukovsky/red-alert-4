// Copyright (c) Red Alert 4 project.
using UnrealBuildTool;

// RA4Recon is part of the deterministic simulation layer: no UObject, no Actor, no
// rendering type. The same sources compile in the standalone CMake harness under
// Tools/HeadlessBuild. Dependencies are the minimum the phases actually use.
public class RA4Recon : ModuleRules
{
	public RA4Recon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicIncludePaths.Add(ModuleDirectory + "/Public");
		PrivateIncludePaths.Add(ModuleDirectory + "/Private");

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "RA4Core", "RA4Content", "RA4FogOfWar" });

		// Matches RA4Simulation: no unity build, so a stray transitive include is
		// caught immediately instead of hiding behind a neighbour's includes.
		bUseUnity = false;
	}
}
