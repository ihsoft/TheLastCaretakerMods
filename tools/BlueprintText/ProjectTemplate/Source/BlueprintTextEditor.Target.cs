using UnrealBuildTool;
using System.Collections.Generic;
public class BlueprintTextEditorTarget : TargetRules
{
    public BlueprintTextEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("BlueprintText");
    }
}
