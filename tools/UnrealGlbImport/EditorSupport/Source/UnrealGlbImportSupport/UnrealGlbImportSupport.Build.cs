using UnrealBuildTool;

public class UnrealGlbImportSupport : ModuleRules
{
    public UnrealGlbImportSupport(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "AssetTools", "UnrealEd" });
    }
}
