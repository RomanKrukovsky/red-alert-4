// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4UI : ModuleRules
{
	public RA4UI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Unity builds merge translation units, and several widget files declare
		// their own palette constants with the same names at namespace scope.
		// Compiling them separately keeps those local, as the authors intended.
		bUseUnity = false;
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"RA4Core",
			"RA4Content",
			"RA4Simulation",
			"RA4Presentation",
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"CommonUI",
			"ModelViewViewModel"
		});
			
		PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore",
            "InputCore",
            "CommonUI",
            "ModelViewViewModel",
            "EnhancedInput"
		});
	}
}
