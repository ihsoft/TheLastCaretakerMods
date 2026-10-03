using UnrealBuildTool;

public class VoyageAssetRegistryWriter : ModuleRules
{
    public VoyageAssetRegistryWriter(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "AssetRegistry", "Json"
        });
    }
}
