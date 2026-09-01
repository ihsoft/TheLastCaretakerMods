// HAND-WRITTEN EDITOR PROJECT SCAFFOLD: builds only the mirror and generator.

using UnrealBuildTool;

public class VoyageEditorTarget : TargetRules
{
    public VoyageEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("Voyage");
        ExtraModuleNames.Add("BoatTotalDieselGenerator");
    }
}
