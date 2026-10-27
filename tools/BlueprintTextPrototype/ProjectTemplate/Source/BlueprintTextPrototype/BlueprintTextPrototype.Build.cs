using UnrealBuildTool;
public class BlueprintTextPrototype : ModuleRules
{
    public BlueprintTextPrototype(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "KismetCompiler", "Json", "JsonUtilities" });
    }
}
