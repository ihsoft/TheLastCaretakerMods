using UnrealBuildTool;
using System.IO;
public class RailgunRuntimeGenerator : ModuleRules
{
    public RailgunRuntimeGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        string GeneratedSettingsDirectory = System.Environment.GetEnvironmentVariable("RAILGUN_GENERATED_SETTINGS_DIR");
        if (string.IsNullOrWhiteSpace(GeneratedSettingsDirectory))
        {
            throw new BuildException("Railgun settings were not generated. Build through Build-Railgun.ps1.");
        }
        PrivateIncludePaths.Add(Path.GetFullPath(GeneratedSettingsDirectory));
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory,
            "../../../../tools/UnrealEditorGeneratorCommon/Public")));
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "KismetCompiler",
            "UMG", "UMGEditor", "AdvancedWidgets", "SlateCore", "SlateBaseRenderer", "Voyage", "EnhancedInput", "InputCore", "InputBlueprintNodes",
            "AssetTools", "AudioEditor" });
    }
}
