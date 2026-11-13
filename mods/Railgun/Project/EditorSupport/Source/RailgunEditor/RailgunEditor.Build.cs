using UnrealBuildTool;

public class RailgunEditor : ModuleRules
{
    public RailgunEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "Json", "UnrealEd", "Voyage"
        });
    }
}
