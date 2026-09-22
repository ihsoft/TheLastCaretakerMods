// HAND-WRITTEN BUILD TOOL SOURCE: editor-only module hosting the GyroKeyboard
// asset-generator commandlets. It is compiled into UnrealEditor, not shipped.

using System.IO;
using UnrealBuildTool;

public class GyroKeyboardGenerator : ModuleRules
{
    public GyroKeyboardGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
