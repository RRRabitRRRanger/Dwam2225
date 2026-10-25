# Petit serveur de fichiers statiques pour tester les pages web de Claude dans le navigateur intégré.
# Usage : powershell -File Tools/Claude/web/serve.ps1 -Root <dossier> -Port 8765
param([string]$Root = $PSScriptRoot, [int]$Port = 8765)
$Listener = New-Object System.Net.HttpListener
$Listener.Prefixes.Add("http://localhost:$Port/")
$Listener.Start()
Write-Host "Serveur sur http://localhost:$Port/ ($Root)"
$Types = @{ ".html" = "text/html; charset=utf-8"; ".js" = "text/javascript"; ".json" = "application/json"; ".css" = "text/css"; ".png" = "image/png" }
while ($Listener.IsListening) {
	$Ctx = $Listener.GetContext()
	$Path = [Uri]::UnescapeDataString($Ctx.Request.Url.AbsolutePath.TrimStart("/"))
	if (-not $Path) { $Path = "index.html" }
	$File = Join-Path $Root $Path
	if (Test-Path $File -PathType Leaf) {
		$Bytes = [IO.File]::ReadAllBytes($File)
		$Ext = [IO.Path]::GetExtension($File)
		$Ctx.Response.ContentType = $(if ($Types[$Ext]) { $Types[$Ext] } else { "application/octet-stream" })
		$Ctx.Response.OutputStream.Write($Bytes, 0, $Bytes.Length)
	} else { $Ctx.Response.StatusCode = 404 }
	$Ctx.Response.Close()
}
