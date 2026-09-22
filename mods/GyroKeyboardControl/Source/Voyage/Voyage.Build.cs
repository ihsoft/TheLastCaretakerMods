// HAND-WRITTEN EDITOR SCAFFOLD: builds only the minimal /Script/Voyage mirror.
// The module exists to compile generated Blueprint assets and is never shipped.

using UnrealBuildTool;

public class Voyage : ModuleRules
{
    public Voyage(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(ModuleDirectory);
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "EnhancedInput", "InputCore" });
    }
}
