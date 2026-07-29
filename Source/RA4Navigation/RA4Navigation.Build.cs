// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4Navigation : ModuleRules
{
	public RA4Navigation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;

		PublicIncludePaths.Add(ModuleDirectory + "/Public");
		PrivateIncludePaths.Add(ModuleDirectory + "/Private");
		
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
