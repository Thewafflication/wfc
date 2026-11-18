<#
.SYNOPSIS
Builds the WPM package of one WFC library from its Release DLLs.

.DESCRIPTION
Used by the release workflow every library repository shares
(.github/workflows/library-release.yml). Reads
<InputRoot>/<arch>/wfc<Library>.dll for x86, x64 and arm64 and writes
wfc-<Library>-any-<version>.zip, the version being the one in VERSION of the
library repository. The package holds one folder per architecture, the
library's README, the WFC license and the WFC library install scripts
(Install-WfcLibrary.ps1, WfcInstallCommon.ps1): WPM's install.cmd hands over
to them, and they copy the DLL to lib\wfc<Library>\<version>\<arch> of the
WFC installation and register it with Windows.

Nothing is signed unless -SigningKey is given, and nothing is published.

.PARAMETER Library
The library: vbrun, mscomctl, mscomctl2, comdlg32 or mswinsck.

.PARAMETER LibraryRoot
The checkout of the library repository.

.PARAMETER InputRoot
The folder with one subfolder per architecture.

.PARAMETER Output
Folder for the package.

.PARAMETER Wpm
Path of wpm.exe.

.PARAMETER SigningKey
Optional WPM private key file; the package is unsigned without it.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('vbrun', 'mscomctl', 'mscomctl2', 'comdlg32', 'mswinsck')]
    [string]$Library,
    [Parameter(Mandatory)][string]$LibraryRoot,
    [string]$InputRoot = 'out/package-input',
    [string]$Output = 'out/packages',
    [string]$Wpm = 'wpm.exe',
    [string]$SigningKey
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$wfcRoot = Split-Path -Parent $PSScriptRoot
$LibraryRoot = (Resolve-Path $LibraryRoot).Path
$name = "wfc-$Library"
$dll = "wfc$Library.dll"
$versionFile = Join-Path $LibraryRoot 'VERSION'
$version = (Get-Content $versionFile -TotalCount 1).Trim()
$revision = (git -C $LibraryRoot rev-parse HEAD).Trim()

$staging = Join-Path $LibraryRoot "out/wpm-staging/$name-any"
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Force -Path $staging, $Output | Out-Null

foreach ($architecture in 'x86', 'x64', 'arm64') {
    $built = Join-Path $InputRoot "$architecture/$dll"
    if (-not (Test-Path $built)) { throw "Missing $built" }
    $target = Join-Path $staging $architecture
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    Copy-Item $built $target
}
Copy-Item (Join-Path $wfcRoot 'LICENSE') $staging
Copy-Item (Join-Path $LibraryRoot 'README.md') $staging
Copy-Item (Join-Path $wfcRoot 'scripts/Install-WfcLibrary.ps1'),
    (Join-Path $wfcRoot 'scripts/WfcInstallCommon.ps1') $staging

New-Item -ItemType Directory -Force -Path (Join-Path $staging '.wpm') |
    Out-Null
$script = '"%~dp0..\Install-WfcLibrary.ps1"'
$arguments = "-Source `"%~dp0..`" -Library wfc$Library"
Set-Content (Join-Path $staging '.wpm/install.cmd') -Encoding ascii -Value @(
    '@echo off'
    "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
        "$arguments || exit /b 1"
    'exit /b 0'
)
Set-Content (Join-Path $staging '.wpm/remove.cmd') -Encoding ascii -Value @(
    '@echo off'
    "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
        "$arguments -Uninstall || exit /b 1"
    'exit /b 0'
)
@(
    "name=$name"
    "version=$version"
    'arch=any'
    'debug=false'
    "description=WFC $Library library $version (registered with Windows)"
    'maintainer=Jordan Waughtal'
    "homepage=https://github.com/Thewafflication/$name"
    "repository=https://github.com/Thewafflication/$name"
    'license=GPL-3.0-or-later'
    "source-version=$version"
    "source-revision=$revision"
) | Set-Content (Join-Path $staging '.wpm/package.txt') -Encoding ascii

$wpmArguments = @('build', $staging, $Output)
if ($SigningKey) {
    $wpmArguments += @('--sign', (Resolve-Path $SigningKey).Path)
}
& $Wpm @wpmArguments
if ($LASTEXITCODE -ne 0) { throw "wpm build failed for $name" }
$package = Join-Path $Output "$name-any-$version.zip"
if (-not (Test-Path $package)) { throw "Missing package: $package" }
$package
