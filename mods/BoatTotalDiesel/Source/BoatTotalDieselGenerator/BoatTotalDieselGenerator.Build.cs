// HAND-WRITTEN BUILD TOOL SOURCE: editor-only asset generator, never shipped.

using UnrealBuildTool;

public class BoatTotalDieselGenerator : ModuleRules
{
    public BoatTotalDieselGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add(System.IO.Path.Combine(
            ModuleDirectory, "..", "..", "..", "..", "tools",
            "UnrealEditorGeneratorCommon", "Public"));

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "BlueprintGraph",
            "Core",
            "CoreUObject",
            "Engine",
            "KismetCompiler",
            "UMG",
            "UMGEditor",
            "UnrealEd",
            "Voyage"
        });
    }
}
