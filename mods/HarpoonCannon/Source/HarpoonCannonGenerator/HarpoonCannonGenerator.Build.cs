// HAND-WRITTEN BUILD TOOL SOURCE: editor-only module hosting the cannon asset
// generator commandlet. It is compiled into UnrealEditor and never shipped.

using UnrealBuildTool;

public class HarpoonCannonGenerator : ModuleRules
{
    public HarpoonCannonGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add(System.IO.Path.Combine(
            ModuleDirectory, "..", "..", "..", "..", "tools",
            "UnrealEditorGeneratorCommon", "Public"));

        PrivateIncludePaths.Add(
            System.IO.Path.Combine(EngineDirectory, "Source", "Editor", "UMGEditor", "Private"));

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "BlueprintGraph",
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "KismetCompiler",
            "MeshConversion",
            "MeshDescription",
            "SlateCore",
            "StaticMeshDescription",
            "UMG",
            "UMGEditor",
            "UnrealEd",
            "Voyage"
        });
    }
}
