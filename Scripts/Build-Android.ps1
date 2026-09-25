param(
    [string]$EngineRoot='F:\Vacancy\Unreal\UE_5.8',
    [string]$SdkRoot=$env:ANDROID_HOME,
    [string]$JavaRoot=$env:JAVA_HOME,
    [switch]$CheckOnly
)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$projectPath=Join-Path $projectRoot 'CountyLine.uproject'
$localTools=Join-Path (Split-Path -Parent $projectRoot) '.android-toolchain'
if (-not $SdkRoot -and (Test-Path -LiteralPath (Join-Path $localTools 'sdk'))) { $SdkRoot=Join-Path $localTools 'sdk' }
if (-not $JavaRoot -and (Test-Path -LiteralPath (Join-Path $localTools 'jdk'))) { $JavaRoot=(Get-ChildItem -LiteralPath (Join-Path $localTools 'jdk') -Directory | Select-Object -First 1).FullName }
$requirements=Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Config/Android/Android_SDK.json') -Raw | ConvertFrom-Json
$missing=[Collections.Generic.List[string]]::new()
if (-not (Test-Path -LiteralPath (Join-Path $EngineRoot 'Engine/Binaries/Android/UnrealGame.target'))) {
    $missing.Add('UE Android target files: Epic Games Launcher > UE 5.8 > Options > Target Platforms > Android.')
}
if (-not $SdkRoot) { $missing.Add('SDK root: pass -SdkRoot or set ANDROID_HOME for this shell.') }
else {
    foreach ($relative in @('platform-tools/adb.exe',"platforms/$($requirements.platforms)/android.jar","build-tools/$($requirements.'build-tools')/aapt2.exe","ndk/$($requirements.ndk)/source.properties")) {
        if (-not (Test-Path -LiteralPath (Join-Path $SdkRoot $relative))) { $missing.Add("Missing SDK component: $relative") }
    }
}
if (-not $JavaRoot -or -not (Test-Path -LiteralPath (Join-Path $JavaRoot 'bin/java.exe'))) { $missing.Add('JDK 21: pass -JavaRoot or set JAVA_HOME for this shell.') }
if ($missing.Count) { throw ("Android build prerequisites missing:`n - "+($missing -join "`n - ")) }
& (Join-Path $JavaRoot 'bin/java.exe') -version
if ($LASTEXITCODE -ne 0) { throw 'Java failed to run.' }
if ($CheckOnly) { Write-Output 'Android prerequisite files are present. No APK has been built.'; return }
# Scoped to this script process; do not change global SDK paths used by other projects.
$oldAndroid=$env:ANDROID_HOME; $oldJava=$env:JAVA_HOME; $oldNdk=$env:NDKROOT
try {
    $env:ANDROID_HOME=$SdkRoot; $env:JAVA_HOME=$JavaRoot; $env:NDKROOT=Join-Path $SdkRoot "ndk/$($requirements.ndk)"
    $archive=Join-Path $projectRoot 'Artifacts/Android'
    $startTime=Get-Date
    $uatArguments=@('BuildCookRun',"-project=$projectPath",'-noP4','-unattended','-utf8output','-platform=Android','-cookflavor=ASTC','-clientconfig=Development','-build','-cook','-map=/Game/Maps/L_JailOffice','-stage','-pak','-iostore','-compressed','-package','-archive',"-archivedirectory=$archive")
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') @uatArguments
    if ($LASTEXITCODE -ne 0) { throw "Android packaging failed with exit code $LASTEXITCODE." }
    $apks=@(Get-ChildItem -LiteralPath $archive -Recurse -Filter '*.apk' | Where-Object LastWriteTime -GE $startTime)
    if (-not $apks.Count) { throw 'Packaging returned success but produced no fresh APK.' }
    foreach ($apk in $apks) {
        Get-FileHash -LiteralPath $apk.FullName -Algorithm SHA256
        Write-Output "APK ready for device testing: $($apk.FullName)"
    }
} finally {
    $env:ANDROID_HOME=$oldAndroid; $env:JAVA_HOME=$oldJava; $env:NDKROOT=$oldNdk
}
