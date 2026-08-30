// HAND-WRITTEN BUILD TOOL SOURCE: editor-only module hosting the DonkLift
// asset-generator commandlets. It is compiled into UnrealEditor, not shipped.

using System.IO;
using UnrealBuildTool;

public class DonkLiftGenerator : ModuleRules
{
    public DonkLiftGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add(
            Path.Combine(EngineDirectory, "Source", "Editor", "UMGEditor", "Private"));
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(
            ModuleDirectory,
            "..", "..", "..", "..",
            "tools", "UnrealEditorGeneratorCommon", "Public")));

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "BlueprintGraph",
            "Core",
            "CoreUObject",
            "Engine",
            "EnhancedInput",
            "InputCore",
            "KismetCompiler",
            "SlateCore",
            "UMG",
            "UMGEditor",
            "UnrealEd",
            "Voyage"
        });
    }
}
