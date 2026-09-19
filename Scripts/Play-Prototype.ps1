param([string]$EngineRoot)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $projectRoot 'CountyLine.uproject'
if (-not $EngineRoot) {
    $manifest = Join-Path $env:ProgramData 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
    if (Test-Path -LiteralPath $manifest) {
        $installs = (Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json).InstallationList
        $EngineRoot = ($installs | Where-Object AppName -eq 'UE_5.8' | Select-Object -First 1).InstallLocation
    }
}
if (-not $EngineRoot) { throw 'Pass -EngineRoot with your Unreal Engine 5.8 directory.' }
$editorPath = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
if (-not (Test-Path -LiteralPath $editorPath)) { throw "UnrealEditor.exe was not found in $EngineRoot" }
if (-not (Test-Path -LiteralPath (Join-Path $projectRoot 'Binaries/Win64/UnrealEditor-CountyLine.dll'))) {
    throw 'Build CountyLineEditor in Development Win64 before running this prototype.'
}
& $editorPath $projectPath '/Game/Maps/L_JailOffice' -game -windowed -ResX=1440 -ResY=900 -nosplash
