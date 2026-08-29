using UnrealBuildTool;

public class Voyage : ModuleRules
{
    public Voyage(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore" });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[]
            {
                "BlueprintGraph",
                "KismetCompiler",
                "UnrealEd"
            });
        }
    }
}
