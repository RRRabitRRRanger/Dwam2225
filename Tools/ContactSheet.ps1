# Assemble des captures en une planche, chaque image sous un bandeau de couleur avec son nom.
# Usage : powershell -File Tools/ContactSheet.ps1 -Out planche.png -Title "..." -Items "a.png|Libellé|#RRGGBB" "b.png|..."

param(
	[Parameter(Mandatory = $true)][string]$Out,
	[string]$Title = "",
	[int]$Tile = 420,
	[Parameter(Mandatory = $true, ValueFromRemainingArguments = $true)][string[]]$Items
)

Add-Type -AssemblyName System.Drawing
$Band = 44
$Head = if ($Title) { 40 } else { 0 }
$W = $Tile * $Items.Count
$H = $Head + $Band + $Tile
$Sheet = New-Object System.Drawing.Bitmap $W, $H
$G = [System.Drawing.Graphics]::FromImage($Sheet)
$G.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$G.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$G.Clear([System.Drawing.Color]::FromArgb(24, 24, 24))
$FontTitle = New-Object System.Drawing.Font "Segoe UI", 15, ([System.Drawing.FontStyle]::Bold)
$Font = New-Object System.Drawing.Font "Segoe UI", 13, ([System.Drawing.FontStyle]::Bold)
$White = [System.Drawing.Brushes]::White
if ($Title) { $G.DrawString($Title, $FontTitle, $White, 10, 8) }

for ($i = 0; $i -lt $Items.Count; $i++) {
	$Parts = $Items[$i] -split "\|"
	$X = $i * $Tile
	$Color = [System.Drawing.ColorTranslator]::FromHtml($(if ($Parts.Count -gt 2) { $Parts[2] } else { "#444444" }))
	$G.FillRectangle((New-Object System.Drawing.SolidBrush $Color), $X, $Head, $Tile, $Band)
	$G.DrawString($Parts[1], $Font, $White, ($X + 10), ($Head + 10))
	$Img = [System.Drawing.Image]::FromFile((Resolve-Path $Parts[0]).Path)
	$G.DrawImage($Img, $X, ($Head + $Band), $Tile, $Tile)
	$Img.Dispose()
	$G.DrawLine((New-Object System.Drawing.Pen ([System.Drawing.Color]::Black), 2), $X, $Head, $X, $H)
}
$Sheet.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$G.Dispose(); $Sheet.Dispose()
Write-Host "Planche : $Out"
