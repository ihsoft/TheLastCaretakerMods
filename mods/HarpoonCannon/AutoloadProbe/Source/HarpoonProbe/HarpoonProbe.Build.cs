using UnrealBuildTool;
using System.IO;
public class HarpoonProbe : ModuleRules
{
    public HarpoonProbe(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory,
            "../../../../../tools/UnrealEditorGeneratorCommon/Public")));
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "KismetCompiler",
            "UMG", "UMGEditor", "SlateCore", "Voyage", "EnhancedInput", "InputCore", "InputBlueprintNodes",
            "AssetTools", "AudioEditor" });
    }
}
