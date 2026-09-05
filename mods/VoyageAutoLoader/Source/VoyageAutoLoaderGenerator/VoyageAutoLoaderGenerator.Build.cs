using UnrealBuildTool;

public class VoyageAutoLoaderGenerator : ModuleRules
{
    public VoyageAutoLoaderGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory,
            "../../../../tools/UnrealEditorGeneratorCommon/Public"));
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "BlueprintGraph",
            "Core",
            "CoreUObject",
            "Engine",
            "KismetCompiler",
            "UnrealEd",
            "UMG",
            "UMGEditor",
            "Voyage"
        });
    }
}
