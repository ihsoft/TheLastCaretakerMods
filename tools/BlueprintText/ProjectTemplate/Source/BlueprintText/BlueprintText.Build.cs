using UnrealBuildTool;
public class BlueprintText : ModuleRules
{
    public BlueprintText(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "KismetCompiler", "Kismet", "Json", "JsonUtilities", "UMG", "UMGEditor", "MovieScene", "SlateCore", "ImageCore", "AssetRegistry" });
    }
}
