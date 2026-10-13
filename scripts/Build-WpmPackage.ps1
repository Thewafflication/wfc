<#
.SYNOPSIS
Builds the WFC WPM packages: the tool and the packaged corpus games.

.DESCRIPTION
Assembles two packages from Release builds (see
planning/MP-0008-release-plan.md):

  wfc-any-<version>.zip        wfc.exe and wfcvbrun.dll for each architecture,
                               LICENSE, README.MD
  wfc-games-any-<version>.zip  each corpus game as a standalone program per
                               architecture, with its PROVENANCE.md, its
                               license and a checksum list

The games are built with that architecture's own wfc.exe (`wfc --build`), so
a program for an architecture can only be built where that wfc.exe runs.
Nothing is signed unless -SigningKey is given, and nothing is published.

.PARAMETER Version
Package version, for example 0.8.0.

.PARAMETER Architectures
Architectures to include; each needs a Release build
(out/build/windows-<arch>-release/Release).

.PARAMETER Wpm
Path of wpm.exe.

.PARAMETER SigningKey
Optional WPM signing key file; the packages are unsigned without it.

.PARAMETER Output
Folder for the packages.

.EXAMPLE
pwsh scripts/Build-WpmPackage.ps1 -Version 0.8.0
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Version,
    [string[]]$Architectures = @('x64'),
    [string]$Wpm = 'wpm.exe',
    [string]$SigningKey,
    [string]$Output = 'out/packages'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $games = @(
        @{ Name = 'captureking'; Project = 'captureking/CaptureKing.vbp' }
        @{ Name = 'clix'; Project = 'clix/src/Clix.vbp' }
        @{ Name = 'connect4'; Project = 'connect4/PConnect4.vbp' }
        @{ Name = 'executioner'; Project = 'executioner/Executioner.vbp' }
        @{ Name = 'pipes'; Project = 'pipes/pipes.vbp' }
        @{ Name = 'restaum'; Project = 'restaum/source/BrainVita.vbp' }
        @{ Name = 'sumboku';
           Project = 'sumboku/v1.4.2/SuMBOku_V_1_4/Sudoku.vbp' }
    )
    $revision = (git rev-parse HEAD).Trim()
    $staging = Join-Path $root 'out/wpm-staging'
    $packages = Join-Path $root $Output
    if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $staging, $packages | Out-Null

    function Get-BuildDirectory([string]$Architecture) {
        $directory = Join-Path $root `
            "out/build/windows-$Architecture-release/Release"
        if (-not (Test-Path (Join-Path $directory 'wfc.exe'))) {
            throw "No Release wfc.exe for ${Architecture}: $directory"
        }
        return $directory
    }

    function Write-PackageInfo([string]$Directory, [string]$Name,
                            [string]$Description) {
        New-Item -ItemType Directory -Force `
            -Path (Join-Path $Directory '.wpm') | Out-Null
        @(
            "name=$Name"
            "version=$Version"
            'arch=any'
            'debug=false'
            "description=$Description"
            'maintainer=Jordan Waughtal'
            'homepage=https://github.com/Thewafflication/wfc'
            'repository=https://github.com/Thewafflication/wfc'
            'license=GPL-3.0-or-later'
            "source-version=$Version"
            "source-revision=$revision"
        ) | Set-Content (Join-Path $Directory '.wpm/package.txt') `
            -Encoding ascii
    }

    function Build-Package([string]$Directory, [string]$Name) {
        $arguments = @('build', $Directory, $packages)
        if ($SigningKey) {
            $arguments += @('--sign', (Resolve-Path $SigningKey).Path)
        }
        & $Wpm @arguments
        if ($LASTEXITCODE -ne 0) { throw "wpm build failed for $Name" }
        $package = Join-Path $packages "$Name-any-$Version.zip"
        if (-not (Test-Path $package)) { throw "Missing package: $package" }
        return $package
    }

    # --- wfc -------------------------------------------------------------
    $tool = Join-Path $staging 'wfc-any'
    foreach ($architecture in $Architectures) {
        $build = Get-BuildDirectory $architecture
        $target = Join-Path $tool $architecture
        New-Item -ItemType Directory -Force -Path $target | Out-Null
        Copy-Item (Join-Path $build 'wfc.exe') $target
        Copy-Item (Join-Path $build 'wfcvbrun.dll') $target
    }
    Copy-Item LICENSE $tool
    Copy-Item README.MD $tool
    Copy-Item planning/compatibility-profile-1.0.md $tool
    Write-PackageInfo $tool 'wfc' 'Waughtal Form Compiler: a VB6 interpreter'
    $toolPackage = Build-Package $tool 'wfc'

    # --- wfc-games -------------------------------------------------------
    $gamesRoot = Join-Path $staging 'wfc-games-any'
    $sums = New-Object System.Collections.Generic.List[string]
    foreach ($game in $games) {
        $source = Join-Path $root "tests/vb6games/$($game.Project)"
        $folder = Join-Path $gamesRoot $game.Name
        New-Item -ItemType Directory -Force -Path $folder | Out-Null
        $gameRoot = "tests/vb6games/$($game.Project.Split('/')[0])"
        Copy-Item (Join-Path $root "$gameRoot/PROVENANCE.md") $folder
        foreach ($license in 'LICENSE', 'LICENSE.txt', 'LICENSE.md') {
            $path = Join-Path $root "$gameRoot/$license"
            if (Test-Path $path) { Copy-Item $path $folder }
        }
        foreach ($architecture in $Architectures) {
            $build = Get-BuildDirectory $architecture
            $program = Join-Path $folder "$($game.Name)-$architecture.exe"
            & (Join-Path $build 'wfc.exe') --build $source -o $program --gui
            if ($LASTEXITCODE -ne 0) {
                throw "wfc --build failed for $($game.Name) ($architecture)"
            }
            $hash = (Get-FileHash $program -Algorithm SHA256).Hash.ToLower()
            $sums.Add("$hash  $($game.Name)/$($game.Name)-$architecture.exe")
        }
    }
    $sums | Set-Content (Join-Path $gamesRoot 'SHA256SUMS') -Encoding ascii
    Copy-Item LICENSE (Join-Path $gamesRoot 'WFC-LICENSE')
    Write-PackageInfo $gamesRoot 'wfc-games' `
        'VB6 games from the vb6games corpus, packaged with WFC'
    $gamesPackage = Build-Package $gamesRoot 'wfc-games'

    $toolPackage
    $gamesPackage
}
finally {
    Pop-Location
}
