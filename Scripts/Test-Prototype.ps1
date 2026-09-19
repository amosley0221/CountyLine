param([Parameter(Mandatory=$true)][string]$EngineRoot)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$projectPath=Join-Path $projectRoot 'CountyLine.uproject'
$commandlet=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$testLog=Join-Path $projectRoot 'Saved/Logs/PrototypeTests.log'
$smokeLog=Join-Path $projectRoot 'Saved/Logs/PrototypeSmoke.log'
& $commandlet $projectPath -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests CountyLine' '-TestExit=Automation Test Queue Empty' "-abslog=$testLog"
if ($LASTEXITCODE -ne 0) { throw 'Unreal automation tests failed; inspect PrototypeTests.log.' }
$testResults=Get-Content -LiteralPath $testLog -Raw
if (($testResults | Select-String -Pattern 'Test Completed\. Result=\{Success\}' -AllMatches).Matches.Count -ne 3) {
    throw 'Expected all three CountyLine tests to complete successfully.'
}
& $commandlet $projectPath '/Game/Maps/L_JailOffice' -game -CLSmokeTest -unattended -nullrhi -nosplash "-abslog=$smokeLog"
if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $smokeLog -SimpleMatch 'CL_SMOKE_RESULT=PASS' -Quiet)) {
    throw 'Runtime smoke test failed; inspect PrototypeSmoke.log.'
}
Write-Output 'County Line automation and runtime smoke checks passed.'
