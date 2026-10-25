# Lit un export T3D de matériau / Material Function (lecture seule) et affiche l'arbre des entrées d'un nœud.
# Usage : powershell -File Tools/T3DTrace.ps1 -T3D Saved/Automation/MF_Light2.t3d -Root MaterialExpressionLinearInterpolate_8 [-Depth 12]
# Les Named Reroute sont suivis jusqu'à leur déclaration.

param(
	[Parameter(Mandatory = $true)][string]$T3D,
	[Parameter(Mandatory = $true)][string]$Root,
	[int]$Depth = 14
)

$Lines = Get-Content $T3D -Encoding UTF8
$Nodes = @{}
$Cur = $null
foreach ($L in $Lines) {
	if ($L -match '^\s{3}Begin Object (?:Class=\S+ )?Name="(MaterialExpression\w+)"') {
		$Cur = $Matches[1]
		if (-not $Nodes.ContainsKey($Cur)) { $Nodes[$Cur] = [ordered]@{ Inputs = @(); Props = @() } }
		continue
	}
	if ($L -match '^\s{3}End Object') { $Cur = $null; continue }
	if (-not $Cur) { continue }
	$T = $L.Trim()
	# Entrées : X=(Expression="...'MF:MaterialExpressionY'" ...)
	foreach ($M in [regex]::Matches($T, '(\w+(?:\(\d+\))?)=\((?:InputName="(\w*)",)?(?:Input=\()?Expression="[^'']*''[^:]*:(MaterialExpression\w+)''"(?:,OutputIndex=(\d+))?')) {
		$Label = if ($M.Groups[2].Value) { $M.Groups[2].Value } else { $M.Groups[1].Value }
		$Nodes[$Cur].Inputs += , @($Label, $M.Groups[3].Value, $M.Groups[4].Value)
	}
	if ($T -match '^(ParameterName|DefaultValue|ConstA|ConstB|R|Name|DeclarationGuid|Declaration|InputName|Code|Desc|MaterialFunction|PrimitiveDataIndex|bUseCustomPrimitiveData|Exponent|BaseReflectFractionIn)=') {
		$Nodes[$Cur].Props += $T.Substring(0, [Math]::Min(160, $T.Length))
	}
}

# Named reroute : usage -> déclaration (même nom)
$DeclByName = @{}
foreach ($K in $Nodes.Keys) {
	if ($K -like "MaterialExpressionNamedRerouteDeclaration*") {
		$N = ($Nodes[$K].Props | Where-Object { $_ -like "Name=*" } | Select-Object -First 1)
		if ($N) { $DeclByName[$N] = $K }
	}
}

$Seen = @{}
function Show([string]$Name, [int]$Level, [string]$Via) {
	$Pad = "  " * $Level
	$Short = $Name -replace "MaterialExpression", ""
	if (-not $Nodes.ContainsKey($Name)) { Write-Output "$Pad$Via$Short (?)"; return }
	$Info = $Nodes[$Name]
	$P = ($Info.Props | Where-Object { $_ -notlike "Declaration*" -and $_ -notlike "DeclarationGuid*" }) -join "  "
	if ($Seen.ContainsKey($Name)) { Write-Output "$Pad$Via$Short  (déjà vu)"; return }
	$Seen[$Name] = $true
	Write-Output "$Pad$Via$Short  $P"
	if ($Level -ge $Depth) { return }
	foreach ($In in $Info.Inputs) {
		Show $In[1] ($Level + 1) "$($In[0]) <- "
	}
	if ($Name -like "MaterialExpressionNamedRerouteUsage*") {
		$D = ($Info.Props | Where-Object { $_ -like "Declaration=*" } | Select-Object -First 1)
		if ($D -match "(MaterialExpressionNamedRerouteDeclaration\w+)") { Show $Matches[1] ($Level + 1) "reroute <- " }
	}
}
Show $Root 0 ""
