param([Parameter(Mandatory=$true)][string]$EngineRoot)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$projectPath=Join-Path $projectRoot 'CountyLine.uproject'
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
if (-not (Test-Path -LiteralPath $editor)) { throw 'Unreal editor executable not found.' }
$trialDesktop=[Environment]::GetFolderPath('Desktop')
$shortcutPath=Join-Path $trialDesktop 'The County Line - Non-canon Playtest.lnk'
$shellObject=New-Object -ComObject WScript.Shell
$shortcut=$shellObject.CreateShortcut($shortcutPath)
if ((Test-Path -LiteralPath $shortcutPath) -and $shortcut.TargetPath -ne $editor) { throw 'An unrelated shortcut already uses this name.' }
$shortcut.TargetPath=$editor
$shortcut.Arguments='"'+$projectPath+'" /Game/Maps/L_JailOffice -game -windowed -ResX=1440 -ResY=900 -nosplash -CLTestMission'
$shortcut.WorkingDirectory=$projectRoot
$shortcut.Description='Disposable non-canon freight-yard playtest. Does not load or write campaign saves.'
$shortcut.Save()
Write-Output $shortcutPath
