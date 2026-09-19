using UnrealBuildTool;

public class CountyLineTarget : TargetRules
{
    public CountyLineTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CountyLine");
    }
}
