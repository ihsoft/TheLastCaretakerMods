// HAND-WRITTEN EDITOR PROJECT SCAFFOLD: builds the Voyage mirror and the
// HarpoonCannonGenerator editor module. Native output is never shipped.

using UnrealBuildTool;

public class VoyageEditorTarget : TargetRules
{
    public VoyageEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("Voyage");
        ExtraModuleNames.Add("HarpoonCannonGenerator");
    }
}
