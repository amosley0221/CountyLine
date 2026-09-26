<#
.SYNOPSIS
Retrieves a benchmark capture from a connected Android device and summarizes it.

.DESCRIPTION
Optionally writes the benchmark command line into the installed app's external
files directory, launches the app, then pulls Saved/Performance off the device
into Saved/Performance/Android on the PC and summarizes the newest capture.

This script installs nothing, changes no signing key, app id, release tag or
project setting, and publishes nothing. Package and install the APK with
Scripts/Build-Android.ps1 first. Nothing here measures anything by itself: the
numbers come from the device, and a PC mobile preview is never an Android result.

.EXAMPLE
Scripts/Performance/Get-AndroidCapture.ps1 -EngineRoot 'F:/Vacancy/Unreal/UE_5.8' -Arm
Writes the benchmark command line, launches the app, waits, then pulls results.

.EXAMPLE
Scripts/Performance/Get-AndroidCapture.ps1 -EngineRoot 'F:/Vacancy/Unreal/UE_5.8'
Pulls whatever captures are already on the device.
#>
param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [string]$Adb,
    [string]$PackageName = 'com.amosley0221.countyline',
    # Writes UECommandLine.txt and launches the app, instead of only pulling.
    [switch]$Arm,
    [double]$WarmUpSeconds = 5,
    [double]$SegmentScale = 1,
    [int]$WaitSeconds = 120
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$python = Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
if (-not (Test-Path -LiteralPath $python)) { throw "Not found: $python" }
if (-not $Adb) {
    $candidates = @((Join-Path $projectRoot '../.android-toolchain/sdk/platform-tools/adb.exe'),
                    (Join-Path $env:LOCALAPPDATA 'Android/Sdk/platform-tools/adb.exe'))
    $Adb = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (-not $Adb) { $Adb = 'adb' }
}
$deviceFiles = "/sdcard/Android/data/$PackageName/files/UnrealGame/CountyLine"

function Invoke-Adb {
    param([string[]]$AdbArguments)
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $output = & $Adb @AdbArguments 2>&1 | Out-String
    $code = $LASTEXITCODE
    $ErrorActionPreference = $previous
    if ($code -ne 0) { throw "adb $($AdbArguments -join ' ') failed: $output" }
    return $output
}

$devices = (Invoke-Adb @('devices')) -split "`n" | Where-Object { $_ -match "`tdevice$" }
if (-not $devices) { throw 'No authorized device. Enable USB debugging, accept the prompt, and check "adb devices".' }
Write-Output "Device: $($devices[0].Trim())"
Write-Output "Android version: $((Invoke-Adb @('shell','getprop','ro.build.version.release')).Trim())"
Write-Output "Model: $((Invoke-Adb @('shell','getprop','ro.product.model')).Trim())"

if ($Arm) {
    # The app reads its command line from this file on launch.
    $commandLine = "../../../CountyLine/CountyLine.uproject /Game/Maps/L_JailOffice -CLPerfBenchmark -CLPerfWarmUp=$WarmUpSeconds -CLPerfScale=$SegmentScale -CLPerfExit"
    $local = Join-Path ([System.IO.Path]::GetTempPath()) 'UECommandLine.txt'
    Set-Content -LiteralPath $local -Value $commandLine -Encoding ascii
    Invoke-Adb @('shell', 'mkdir', '-p', $deviceFiles) | Out-Null
    Invoke-Adb @('push', $local, "$deviceFiles/UECommandLine.txt") | Out-Null
    Remove-Item -LiteralPath $local -Force
    Write-Output 'Wrote UECommandLine.txt. Launching; keep the screen on and do not switch apps.'
    Invoke-Adb @('shell', 'monkey', '-p', $PackageName, '-c', 'android.intent.category.LAUNCHER', '1') | Out-Null
    Start-Sleep -Seconds $WaitSeconds
    Write-Output 'Remove the file afterwards so normal play is unaffected:'
    Write-Output "  $Adb shell rm $deviceFiles/UECommandLine.txt"
}

$destination = Join-Path $projectRoot 'Saved/Performance/Android'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
Invoke-Adb @('pull', "$deviceFiles/Saved/Performance", $destination) | Out-Null
$captures = Get-ChildItem -LiteralPath (Join-Path $destination 'Performance') -Directory -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime
if (-not $captures) { throw "No captures under $deviceFiles/Saved/Performance. Confirm the run finished on the device." }
$capture = $captures[-1].FullName
Write-Output "Capture: $capture"

$previousPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
& $python (Join-Path $PSScriptRoot 'summarize_benchmark.py') $capture --json (Join-Path $capture 'summary.json')
$parserExit = $LASTEXITCODE
$ErrorActionPreference = $previousPreference
if ($parserExit -ne 0) { throw 'The capture could not be summarized.' }
Write-Output 'This is a physical Android capture only if it came from the device above.'
