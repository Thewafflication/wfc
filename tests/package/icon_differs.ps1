# Exits 0 when the icons of two executables are not the same image.
param(
    [Parameter(Mandatory)][string]$First,
    [Parameter(Mandatory)][string]$Second
)
Add-Type -AssemblyName System.Drawing
function Pixels([string]$Path) {
    $icon = [System.Drawing.Icon]::ExtractAssociatedIcon($Path)
    $bitmap = $icon.ToBitmap()
    $values = for ($y = 0; $y -lt $bitmap.Height; $y += 4) {
        for ($x = 0; $x -lt $bitmap.Width; $x += 4) {
            $bitmap.GetPixel($x, $y).ToArgb()
        }
    }
    $values -join ','
}
if ((Pixels $First) -eq (Pixels $Second)) {
    Write-Error 'the icons are identical'
    exit 1
}
