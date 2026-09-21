param([Parameter(Mandatory=$true)][string]$EngineRoot,[switch]$Review)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$projectPath=Join-Path $projectRoot 'CountyLine.uproject'
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$log=Join-Path $projectRoot 'Saved/Logs/FreightTrial.log'
$renderArguments=@('-nullrhi')
if ($Review) { $renderArguments=@('-RenderOffscreen','-ResX=1280','-ResY=720','-CLTrialReview') }
& $editor $projectPath '/Game/Maps/L_JailOffice' -game -Multiprocess -unattended -nosplash -CLTestMission -CLTrialSmoke @renderArguments "-abslog=$log"
if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $log -Pattern 'CL_TRIAL_RESULT=PASS' -SimpleMatch -Quiet)) {
    throw 'Freight trial failed; inspect Saved/Logs/FreightTrial.log.'
}
Write-Output 'Non-canon freight trial runtime checks passed.'
