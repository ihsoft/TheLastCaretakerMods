using UnrealBuildTool;
public class BPTNativeDefaults : ModuleRules
{
    public BPTNativeDefaults(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
    }
}
