// Copyright (c) Red Alert 4 project.
using UnrealBuildTool;

// RA4Replay is deliberately engine-light: it depends on Core only for the build system and
// uses no UObject, no Actor and no rendering type. The same sources compile in the
// standalone CMake harness under Tools/HeadlessBuild, which is what lets the
// determinism suite run in CI without the editor.
public class RA4Replay : ModuleRules
{
	public RA4Replay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;

		PublicIncludePaths.Add(ModuleDirectory + "/Public");
		PrivateIncludePaths.Add(ModuleDirectory + "/Private");

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "RA4Core", "RA4Content", "RA4Simulation" });

		// No exceptions and no RTTI in the simulation: both add non-determinism
		// risk through allocation order and are not needed by any of this code.
		bUseUnity = false;
	}
}
