// HAND-WRITTEN BUILD TOOL SOURCE: editor-only module hosting the StableGyro
// asset-generator commandlets. It is compiled into UnrealEditor, not shipped.

using System.IO;
using UnrealBuildTool;

public class StableGyroGenerator : ModuleRules
{
    public StableGyroGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        string GeneratedSettingsDirectory = Path.GetFullPath(Path.Combine(
            ModuleDirectory,
            "..", "..", "Intermediate", "GeneratedSettings"));
        string GeneratedSettingsHeader = Path.Combine(
            GeneratedSettingsDirectory, "StableGyroSettings.generated.h");
        if (!File.Exists(GeneratedSettingsHeader))
        {
            throw new BuildException(
                "StableGyro settings were not generated. Build through Build-StableGyro.ps1.");
        }
        PrivateIncludePaths.Add(GeneratedSettingsDirectory);

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
            "UnrealEd",
            "Voyage"
        });
    }
}
