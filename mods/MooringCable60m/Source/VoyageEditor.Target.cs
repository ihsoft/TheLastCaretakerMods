using UnrealBuildTool;
public class VoyageEditorTarget : TargetRules
{
    public VoyageEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new[] { "Voyage", "MooringGenerator" });
    }
}
