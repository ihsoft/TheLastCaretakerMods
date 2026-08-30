// HAND-WRITTEN EDITOR PROJECT SCAFFOLD: target for the mirror project only.
// Native output is a generation aid and is never part of the released mod.

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
