// Copyright (c) Red Alert 4 project.

using UnrealBuildTool;

public class RA4UI : ModuleRules
{
	public RA4UI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] {
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
            "CommonUI",
            "ModelViewViewModel",
            "EnhancedInput"
		});
	}
}
