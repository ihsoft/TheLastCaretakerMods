using UnrealBuildTool;
using System.IO;
public class MooringGenerator : ModuleRules
{
    public MooringGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../tools/UnrealEditorGeneratorCommon/Public")));
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Voyage", "UnrealEd", "BlueprintGraph", "KismetCompiler" });
    }
}
