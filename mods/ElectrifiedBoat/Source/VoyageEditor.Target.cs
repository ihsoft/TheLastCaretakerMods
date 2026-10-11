using UnrealBuildTool;
using System.Collections.Generic;

public class VoyageEditorTarget : TargetRules
{
    public VoyageEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "Voyage", "ElectrifiedBoatGenerator" });
    }
}
