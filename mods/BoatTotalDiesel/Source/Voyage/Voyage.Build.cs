// HAND-WRITTEN EDITOR SCAFFOLD: minimal /Script/Voyage mirror only.

using UnrealBuildTool;

public class Voyage : ModuleRules
{
    public Voyage(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(ModuleDirectory);
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG" });
    }
}
