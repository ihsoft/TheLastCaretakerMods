using UnrealBuildTool;
using System.Collections.Generic;

public class VoyageTarget : TargetRules
{
    public VoyageTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Voyage");
    }
}
