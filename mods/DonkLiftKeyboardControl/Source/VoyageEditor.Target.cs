// HAND-WRITTEN EDITOR PROJECT SCAFFOLD: builds the Voyage mirror together with
// the DonkLiftGenerator editor-tools module. Native output is never shipped.

using UnrealBuildTool;

public class VoyageEditorTarget : TargetRules
{
    public VoyageEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("Voyage");
        ExtraModuleNames.Add("DonkLiftGenerator");
    }
}
