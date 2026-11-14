using UnrealBuildTool;

public class UnrealCookSupport : ModuleRules
{
    public UnrealCookSupport(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd" });
    }
}
