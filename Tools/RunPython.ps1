# Lance un script Python d'éditeur, éditeur AVEC sa fenêtre (décision de l'auteur), puis le ferme.
# Usage : powershell -File Tools/RunPython.ps1 Tools/Claude/create_joint_blend_material.py
# Règle : les scripts de Claude ne créent des assets que dans Content/_Claude/.

param([Parameter(Mandatory = $true)][string]$Script)

$ErrorActionPreference = "Stop"
$Project = (Resolve-Path "$PSScriptRoot\..\Dwam2225.uproject").Path
$Root = Split-Path $Project
$Editor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$ScriptPath = (Resolve-Path $Script).Path
$Log = Join-Path $Root "Saved\Automation\RunPython.log"

if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) {
	Write-Host "Un éditeur Unreal est déjà ouvert : ferme-le avant de lancer le script." -ForegroundColor Yellow
	exit 2
}
New-Item -ItemType Directory -Force (Split-Path $Log) | Out-Null

# Le script s'exécute une fois l'éditeur prêt, puis l'éditeur se ferme
$Wrapper = Join-Path $env:TEMP "claude_run_python.py"
@"
import unreal, runpy, traceback
try:
    runpy.run_path(r'''$ScriptPath''', run_name='__main__')
except Exception:
    unreal.log_error('CLAUDE: ' + traceback.format_exc())
unreal.SystemLibrary.quit_editor()
"@ | Set-Content -Encoding UTF8 $Wrapper

$Args = @("`"$Project`"", "-ExecutePythonScript=`"$Wrapper`"", "-abslog=`"$Log`"", "-nosplash", "-unattended", "-nopause")
Write-Host "Script : $ScriptPath (fenêtre de l'éditeur visible, fermeture automatique)"
Start-Process -FilePath $Editor -ArgumentList $Args -Wait | Out-Null
Select-String -Path $Log -Pattern "CLAUDE:|LogPython: Error|Error:.*\.py" | ForEach-Object { $_.Line }
