<#
.SYNOPSIS
Records a scripted play session of a project as an animated GIF.

.DESCRIPTION
Runs the project in the Win32 host with a ui-script, takes a screenshot of the
form after every click, drag and type line, and joins the frames with ffmpeg.
The project is run from a copy, so it does not write into the source folder.

.EXAMPLE
scripts\New-PlayGif.ps1 -Project tests\vb6games\connect4\PConnect4.vbp `
  -Script tests\projects\connect4-play.ui -Form FMain `
  -Output evidence\screenshots\play-connect4.gif
#>
param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$Script,
    [Parameter(Mandatory)][string]$Form,
    [Parameter(Mandatory)][string]$Output,
    [string]$Wfc = 'out\build\windows-x64-debug\Debug\wfc.exe',
    [int]$FrameMilliseconds = 400,
    [int]$Width = 0,
    [int]$ExitAfter = 60000,
    [string]$FrameOn = '^(click|drag|type|key)\s'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$work = Join-Path $env:TEMP ('wfc-gif-' + [guid]::NewGuid().ToString('N'))
$frames = Join-Path $work 'frames'
$copy = Join-Path $work 'project'
New-Item -ItemType Directory -Path $frames, $copy | Out-Null
$projectFull = (Resolve-Path $Project).Path
Copy-Item -Path (Join-Path (Split-Path $projectFull) '*') -Destination $copy `
    -Recurse -Force

# A frame after each action, so the picture changes between frames.
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("screenshot $Form f000")
$count = 0
foreach ($line in Get-Content $Script) {
    $lines.Add($line)
    if ($line -match $FrameOn) {
        $count++
        $lines.Add(('screenshot {0} f{1:D3}' -f $Form, $count))
    }
}
$modified = Join-Path $work 'play.ui'
Set-Content -Path $modified -Value $lines -Encoding ascii

$arguments = @(
    (Join-Path $copy (Split-Path -Leaf $projectFull)), '--gui',
    '--exit-after', $ExitAfter, '--ui-script', $modified,
    '--screenshots', $frames)
& $Wfc @arguments | Out-Null
if ($LASTEXITCODE -ne 0) { throw "wfc exited with $LASTEXITCODE" }

$first = Get-ChildItem $frames -Filter "$Form-f*.png" | Select-Object -First 1
if ($null -eq $first) { throw 'no frames were captured' }
$scale = if ($Width -gt 0) { "scale=${Width}:-1:flags=lanczos," } else { '' }
$filter = "${scale}split[a][b];[a]palettegen=stats_mode=diff[p];" +
    '[b][p]paletteuse=dither=bayer:bayer_scale=5'
$rate = 1000.0 / $FrameMilliseconds
$outputFull = Join-Path $root $Output
New-Item -ItemType Directory -Force -Path (Split-Path $outputFull) | Out-Null
$pattern = Join-Path $frames "$Form-f%03d.png"
& ffmpeg -y -loglevel error -framerate $rate -i $pattern `
    -vf $filter -loop 0 $outputFull
if ($LASTEXITCODE -ne 0) { throw "ffmpeg exited with $LASTEXITCODE" }
Remove-Item -Recurse -Force $work
'{0}: {1} frames' -f $Output, ($count + 1)
