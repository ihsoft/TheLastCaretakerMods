using UnrealBuildTool;

public class VoyageAssetRegistryWriterEditorTarget : TargetRules
{
    public VoyageAssetRegistryWriterEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("VoyageAssetRegistryWriter");
    }
}
