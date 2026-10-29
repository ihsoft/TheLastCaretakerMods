using UnrealBuildTool;
using System.Collections.Generic;
public class BlueprintTextTarget : TargetRules
{
    public BlueprintTextTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("BlueprintText");
    }
}
