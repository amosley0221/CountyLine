param([Parameter(Mandatory=$true)][string]$EngineRoot)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$projectPath=Join-Path $projectRoot 'CountyLine.uproject'
$commandlet=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$testLog=Join-Path $projectRoot 'Saved/Logs/PrototypeTests.log'
$smokeLog=Join-Path $projectRoot 'Saved/Logs/PrototypeSmoke.log'
# Every CountyLine automation test that must run and succeed. Add new tests here.
$expectedTests=@(
    'CountyLine.Interaction.RangeAndView',
    'CountyLine.Report.SaveRoundTrip',
    'CountyLine.Report.SubmissionAndCarbon',
    'CountyLine.Save.Validation.Acceptance',
    'CountyLine.Save.Validation.CompleteCopy',
    'CountyLine.Save.Validation.RejectionKeepsState',
    'CountyLine.Save.Validation.Rejections'
)
& $commandlet $projectPath -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests CountyLine' '-TestExit=Automation Test Queue Empty' "-abslog=$testLog"
if ($LASTEXITCODE -ne 0) { throw 'Unreal automation tests failed; inspect PrototypeTests.log.' }
$completed=@{}
foreach ($match in (Select-String -LiteralPath $testLog -Pattern 'Test Completed\. Result=\{(\w+)\} Name=\{[^}]*\} Path=\{([^}]+)\}')) {
    $path=$match.Matches[0].Groups[2].Value
    if (-not $completed.ContainsKey($path)) { $completed[$path]=@() }
    $completed[$path]+=$match.Matches[0].Groups[1].Value
}
$problems=@()
foreach ($name in $expectedTests) {
    if (-not $completed.ContainsKey($name)) { $problems+="$name did not run" }
    elseif ($completed[$name].Count -ne 1) { $problems+="$name ran $($completed[$name].Count) times" }
}
foreach ($name in $completed.Keys) {
    foreach ($result in $completed[$name]) { if ($result -ne 'Success') { $problems+="$name finished with $result" } }
    if ($expectedTests -notcontains $name) { $problems+="$name is not listed in Test-Prototype.ps1" }
}
if ($problems.Count -gt 0) {
    throw "CountyLine automation problems (inspect PrototypeTests.log):`n  $($problems -join "`n  ")"
}
Write-Output "All $($expectedTests.Count) expected CountyLine automation tests succeeded."
& $commandlet $projectPath '/Game/Maps/L_JailOffice' -game -CLSmokeTest -unattended -nullrhi -nosplash "-abslog=$smokeLog"
if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $smokeLog -SimpleMatch 'CL_SMOKE_RESULT=PASS' -Quiet)) {
    throw 'Runtime smoke test failed; inspect PrototypeSmoke.log.'
}
Write-Output 'County Line automation and runtime smoke checks passed.'
