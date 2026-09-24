// HAND-WRITTEN BUILD TOOL SOURCE: editor-only module hosting the GyroKeyboard
// asset-generator commandlets. It is compiled into UnrealEditor, not shipped.

using System.IO;
using UnrealBuildTool;

public class GyroKeyboardGenerator : ModuleRules
{
    public GyroKeyboardGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        string GeneratedSettingsDirectory = System.Environment.GetEnvironmentVariable(
            "GYRO_KEYBOARD_GENERATED_SETTINGS_DIR");
        if (string.IsNullOrWhiteSpace(GeneratedSettingsDirectory))
        {
            throw new BuildException(
                "GyroKeyboardControl settings were not generated. Build through Build-GyroKeyboardControl.ps1.");
        }
        PrivateIncludePaths.Add(Path.GetFullPath(GeneratedSettingsDirectory));

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
