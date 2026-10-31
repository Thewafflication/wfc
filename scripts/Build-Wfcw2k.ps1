<#
.SYNOPSIS
Builds wfcw2k.exe, the small runtime for programs that run on Windows 2000.
.DESCRIPTION
Builds WCRT for x86 (the static library and the console startup object, from
the wcrt submodule) and links src/win2000/wfcw2k.c against them with the x86
TinyCC. The result needs no C++ runtime and imports only KERNEL32 functions of
Windows 2000.
.PARAMETER TinyCc
The i386-win32-tcc.exe of TinyCC. Default: the newest in a WPM installation of
tinycc (C:\Program Files\TinyCC).
.PARAMETER Output
The wfcw2k.exe to write.
.PARAMETER Work
A folder for the WCRT build.
.EXAMPLE
pwsh -File scripts/Build-Wfcw2k.ps1 -Output out/w2k/wfcw2k.exe
#>
[CmdletBinding()]
param(
    [string]$TinyCc,
    [Parameter(Mandatory)][string]$Output,
    [string]$Work
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $TinyCc) {
    $newest = Get-ChildItem 'C:\Program Files\TinyCC' -Directory `
            -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        Where-Object {
            Test-Path (Join-Path $_.FullName 'i386-win32-tcc.exe')
        } |
        Select-Object -First 1
    if (-not $newest) {
        throw 'TinyCC (i386-win32-tcc.exe) was not found; pass -TinyCc.'
    }
    $TinyCc = Join-Path $newest.FullName 'i386-win32-tcc.exe'
}
$TinyCc = (Resolve-Path -LiteralPath $TinyCc).Path
$compilerRoot = Split-Path -Parent $TinyCc
if (-not $Work) { $Work = Join-Path $root 'out/wcrt-w2k' }
$Output = [IO.Path]::GetFullPath($Output, (Get-Location).Path)
$wcrt = Join-Path $root 'wcrt'

# WCRT takes its output folder relative to its own root.
$Work = [IO.Path]::GetFullPath($Work, (Get-Location).Path)
$relative = [IO.Path]::GetRelativePath($wcrt, $Work)
& pwsh -NoProfile -File (Join-Path $wcrt 'tools/build-wcrt.ps1') `
    -Architecture x86 -Configuration Release -TinyCc $TinyCc `
    -OutputRoot $relative | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'The WCRT build failed.' }

$build = Join-Path $Work 'x86/Release'
New-Item -ItemType Directory -Force (Split-Path -Parent $Output) | Out-Null
$arguments = @(
    '-Wall', '-nostdlib', '-Wl,-nostdlib', '-Wl,-subsystem=console',
    '-I', (Join-Path $build 'include'),
    (Join-Path $build 'wcrt-startup-console.o'),
    (Join-Path $root 'src/win2000/wfcw2k.c'),
    (Join-Path $build 'libwcrt.a'),
    (Join-Path $compilerRoot 'lib/i386-win32-libtcc1.a'),
    (Join-Path $compilerRoot 'lib/kernel32.def'),
    '-o', $Output
)
& $TinyCc @arguments
if ($LASTEXITCODE -ne 0) { throw 'Linking wfcw2k.exe failed.' }
Write-Host "wfcw2k.exe: $Output"
