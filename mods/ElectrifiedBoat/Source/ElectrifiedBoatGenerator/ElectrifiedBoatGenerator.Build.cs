using UnrealBuildTool;

public class ElectrifiedBoatGenerator : ModuleRules
{
    public ElectrifiedBoatGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "Voyage", "UnrealEd",
            "BlueprintGraph", "KismetCompiler"
        });
    }
}
