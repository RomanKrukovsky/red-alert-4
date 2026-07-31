using UnrealBuildTool;

public class RAAI : ModuleRules
{
    public RAAI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTasks",
            "AIModule",
            "NavigationSystem",
            "GameplayAbilities",
            "GameplayTags",
            "MassCommon",
            "MassSimulation",
            "MassAIBehavior",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "StructUtils",
            "Projects"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "DeveloperSettings",
            "HTTP",
            "Json",
            "JsonUtilities"
        });

        PublicIncludePaths.AddRange(new string[]
        {
            ModuleDirectory + "/Public",
            ModuleDirectory + "/Public/AI",
            ModuleDirectory + "/Public/Planning",
            ModuleDirectory + "/Public/Intelligence",
            ModuleDirectory + "/Public/Managers"
        });

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.Add("UnrealEd");
        }

        // Enable C++17 features
        CppStandard = CppStandardVersion.Cpp20;
    }
}