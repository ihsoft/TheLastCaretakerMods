using UnrealBuildTool;
using System.IO;
public class RailgunRuntimeGenerator : ModuleRules
{
    public RailgunRuntimeGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory,
            "../../../../tools/UnrealEditorGeneratorCommon/Public")));
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "KismetCompiler",
            "UMG", "UMGEditor", "SlateCore", "SlateBaseRenderer", "Voyage", "EnhancedInput", "InputCore", "InputBlueprintNodes",
            "AssetTools", "AudioEditor" });
    }
}
