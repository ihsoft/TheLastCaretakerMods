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
