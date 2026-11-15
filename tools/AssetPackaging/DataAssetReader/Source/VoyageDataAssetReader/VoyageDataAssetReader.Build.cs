using UnrealBuildTool;

public class VoyageDataAssetReader : ModuleRules
{
    public VoyageDataAssetReader(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "Json" });
    }
}
