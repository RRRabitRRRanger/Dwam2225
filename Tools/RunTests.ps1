# Lance les tests automatiques Unreal du projet, éditeur AVEC sa fenêtre (décision de l'auteur, 2026-10-25),
# puis affiche le résumé. L'éditeur se ferme tout seul à la fin des tests.
#
#   powershell -File Tools/RunTests.ps1 CharacterKit
#   powershell -File Tools/RunTests.ps1 "CharacterKit+Rope"
#
# Résultats : Saved/Automation/Reports/index.json, journal : Saved/Automation/RunTests.log,
# captures des tests visuels : Saved/Automation/<Plugin>/.

param(
	[string]$Filter = "CharacterKit"
)

$ErrorActionPreference = "Stop"
$Project = (Resolve-Path "$PSScriptRoot\..\Dwam2225.uproject").Path
$Root = Split-Path $Project
$Editor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$Reports = Join-Path $Root "Saved\Automation\Reports"
$Log = Join-Path $Root "Saved\Automation\RunTests.log"

if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) {
	Write-Host "Un éditeur Unreal est déjà ouvert : ferme-le avant de lancer les tests." -ForegroundColor Yellow
	exit 2
}

if (Test-Path $Reports) { Remove-Item $Reports -Recurse -Force }
New-Item -ItemType Directory -Force $Reports | Out-Null

$TestList = ($Filter -split "\+") -join "+"
$Args = @(
	"`"$Project`"",
	"-ExecCmds=`"Automation RunTests $TestList`"",
	"-TestExit=`"Automation Test Queue Empty`"",
	"-ReportExportPath=`"$Reports`"",
	"-abslog=`"$Log`"",
	"-nosplash",
	"-unattended",
	"-nopause"
)
Write-Host "Tests : $TestList (fenêtre de l'éditeur visible, fermeture automatique)"
$P = Start-Process -FilePath $Editor -ArgumentList $Args -PassThru -Wait

$Index = Join-Path $Reports "index.json"
if (-not (Test-Path $Index)) {
	Write-Host "Pas de rapport : voir $Log" -ForegroundColor Red
	exit 1
}
$Report = Get-Content $Index -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($T in $Report.tests) {
	$Color = if ($T.state -eq "Success") { "Green" } else { "Red" }
	Write-Host ("[{0}] {1}" -f $T.state, $T.fullTestPath) -ForegroundColor $Color
	foreach ($E in $T.entries) {
		$Ev = $E.event
		Write-Host ("    {0}: {1}" -f $Ev.type, $Ev.message)
	}
}
Write-Host ("Réussis : {0}, échoués : {1}" -f $Report.succeeded, $Report.failed)
if ($Report.failed -gt 0) { exit 1 }
