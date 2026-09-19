using UnrealBuildTool;

public class CountyLineEditorTarget : TargetRules
{
    public CountyLineEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CountyLine");
    }
}
