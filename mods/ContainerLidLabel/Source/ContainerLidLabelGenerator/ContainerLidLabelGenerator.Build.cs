using UnrealBuildTool;

public class ContainerLidLabelGenerator : ModuleRules
{
    public ContainerLidLabelGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "BlueprintEditorLibrary", "BlueprintGraph", "Core", "CoreUObject", "Engine", "Kismet",
            "KismetCompiler", "SlateCore", "UMG", "UMGEditor", "UnrealEd", "Voyage"
        });
    }
}
