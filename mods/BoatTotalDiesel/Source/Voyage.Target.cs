// HAND-WRITTEN EDITOR PROJECT SCAFFOLD: native output is never shipped.

using UnrealBuildTool;

public class VoyageTarget : TargetRules
{
    public VoyageTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("Voyage");
    }
}
