using UnrealBuildTool;
public class IniParserTests : ModuleRules
{
    public IniParserTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "Json", "Voyage" });
    }
}
