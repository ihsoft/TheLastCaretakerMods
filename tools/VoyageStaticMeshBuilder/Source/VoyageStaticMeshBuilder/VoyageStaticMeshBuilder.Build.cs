using UnrealBuildTool;

public class VoyageStaticMeshBuilder : ModuleRules
{
    public VoyageStaticMeshBuilder(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InterchangeCore",
            "InterchangeEngine",
            "InterchangePipelines",
            "Json",
            "MeshDescription",
            "StaticMeshDescription",
            "UnrealEd"
        });
    }
}
