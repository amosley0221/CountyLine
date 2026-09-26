<#
.SYNOPSIS
Runs the opt-in PC performance benchmark and summarizes the capture.

.DESCRIPTION
Launches the built game with -CLPerfBenchmark, waits for it to finish the camera
route, then summarizes the newest capture under Saved/Performance. The benchmark
never reads or writes the player's save slot, and this script changes no project
settings, renderer configuration or packaging.

Build the editor target first; this script does not build.

.EXAMPLE
Scripts/Performance/Run-Benchmark.ps1 -EngineRoot 'F:/Vacancy/Unreal/UE_5.8'

.EXAMPLE
Scripts/Performance/Run-Benchmark.ps1 -EngineRoot 'F:/Vacancy/Unreal/UE_5.8' -ResX 1280 -ResY 720 -FrameCap 0 -MobilePreview
#>
param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [int]$ResX = 1920,
    [int]$ResY = 1080,
    [double]$WarmUpSeconds = 5,
    [double]$SegmentScale = 1,
    # 0 leaves the frame rate uncapped, which is usually what you want to measure.
    [int]$FrameCap = 0,
    [switch]$MobilePreview,
    [switch]$KeepOpen,
    [int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectPath = Join-Path $projectRoot 'CountyLine.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$python = Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
foreach ($required in @($projectPath, $editor, $python)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Not found: $required" }
}
$captureRoot = Join-Path $projectRoot 'Saved/Performance'
$before = @()
if (Test-Path -LiteralPath $captureRoot) { $before = Get-ChildItem -LiteralPath $captureRoot -Directory | Select-Object -ExpandProperty FullName }

$arguments = @("`"$projectPath`"", '/Game/Maps/L_JailOffice', '-game', '-CLPerfBenchmark',
    "-CLPerfWarmUp=$WarmUpSeconds", "-CLPerfScale=$SegmentScale",
    '-windowed', "-ResX=$ResX", "-ResY=$ResY", '-nosplash', '-unattended')
if (-not $KeepOpen) { $arguments += '-CLPerfExit' }
if ($MobilePreview) { $arguments += '-CLMobilePreview' }
# A frame cap changes what is measured, so it is recorded in the capture metadata.
if ($FrameCap -gt 0) { $arguments += "-execcmds=t.MaxFPS $FrameCap" }

Write-Output "Launching benchmark: $ResX x $ResY, warm-up $WarmUpSeconds s, scale $SegmentScale, frame cap $(if ($FrameCap -gt 0) { $FrameCap } else { 'uncapped' })"
Write-Output 'Leave the window focused and do not resize it while the route runs.'
$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    throw "The benchmark did not finish within $TimeoutSeconds s; inspect Saved/Logs/CountyLine.log."
}

$after = Get-ChildItem -LiteralPath $captureRoot -Directory -ErrorAction SilentlyContinue |
    Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime
if (-not $after) { throw "No new capture appeared under $captureRoot; the run aborted. Search Saved/Logs/CountyLine.log for CL_PERF_ABORT." }
$capture = $after[-1].FullName
Write-Output "Capture: $capture"

$previousPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
& $python (Join-Path $PSScriptRoot 'summarize_benchmark.py') $capture --json (Join-Path $capture 'summary.json')
$parserExit = $LASTEXITCODE
$ErrorActionPreference = $previousPreference
if ($parserExit -ne 0) { throw 'The capture could not be summarized.' }
