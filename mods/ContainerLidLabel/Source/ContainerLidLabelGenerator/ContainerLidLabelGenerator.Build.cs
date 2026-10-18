using UnrealBuildTool;
using System.IO;

public class ContainerLidLabelGenerator : ModuleRules
{
    public ContainerLidLabelGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        string GeneratedSettingsDirectory = Path.GetFullPath(Path.Combine(
            ModuleDirectory, "../../Intermediate/GeneratedSettings"));
        if (!File.Exists(Path.Combine(GeneratedSettingsDirectory,
            "ContainerLidLabelSettings.generated.h")))
            throw new BuildException("ContainerLidLabel settings were not generated through the public producer.");
        PrivateIncludePaths.Add(GeneratedSettingsDirectory);
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(
            ModuleDirectory, "../../../../tools/VoyageModSettings/Public")));
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "BlueprintEditorLibrary", "BlueprintGraph", "Core", "CoreUObject", "Engine", "Kismet",
            "KismetCompiler", "SlateCore", "UMG", "UMGEditor", "UnrealEd", "Voyage"
        });
    }
}
