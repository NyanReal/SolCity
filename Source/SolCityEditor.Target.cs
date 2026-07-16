using UnrealBuildTool;
using System.Collections.Generic;

public class SolCityEditorTarget : TargetRules
{
    public SolCityEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SolCity");
    }
}
