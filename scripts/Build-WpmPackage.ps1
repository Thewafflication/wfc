<#
.SYNOPSIS
Builds the WFC WPM packages: the tool and the packaged corpus games.

.DESCRIPTION
Two stages (see planning/MP-0008-release-plan.md):

  -Collect   Run on each architecture's own machine: copies that
             architecture's Release wfc.exe and wfcvbrun.dll and builds each
             corpus game with `wfc --build` into out/package-input/<arch>.
             A program for an architecture can only be built where that
             architecture's wfc.exe runs.

  (default)  Assembles from out/package-input/<arch> for every listed
             architecture:
               wfc-any-<version>.zip        the tool for each architecture,
                                            LICENSE, README.MD
               wfc-games-any-<version>.zip  each game per architecture with
                                            its PROVENANCE.md, license and a
                                            checksum list

Nothing is signed unless -SigningKey is given (CI passes the key from the
protected release environment), and nothing is published.

.PARAMETER Version
Package version, for example 0.8.0.

.PARAMETER Collect
Collect stage for one architecture (see -Architectures).

.PARAMETER Architectures
Architectures to collect or to include; Collect needs the Release build
(out/build/windows-<arch>-release/Release).

.PARAMETER Wpm
Path of wpm.exe.

.PARAMETER SigningKey
Optional WPM private key file; the packages are unsigned without it.

.PARAMETER InputRoot
Where the collect stage writes and the assemble stage reads.

.PARAMETER Output
Folder for the packages.

.EXAMPLE
pwsh scripts/Build-WpmPackage.ps1 -Collect -Architectures x64
pwsh scripts/Build-WpmPackage.ps1 -Version 0.8.0 -Architectures x64
#>
[CmdletBinding()]
param(
    [string]$Version,
    [switch]$Collect,
    [string[]]$Architectures = @('x64'),
    [string]$Wpm = 'wpm.exe',
    [string]$SigningKey,
    [string]$InputRoot = 'out/package-input',
    [string]$Output = 'out/packages'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path -Parent $PSScriptRoot
$games = @(
    @{ Name = 'captureking'; Project = 'captureking/CaptureKing.vbp' }
    @{ Name = 'clix'; Project = 'clix/src/Clix.vbp' }
    @{ Name = 'connect4'; Project = 'connect4/PConnect4.vbp' }
    @{ Name = 'executioner'; Project = 'executioner/Executioner.vbp' }
    @{ Name = 'pipes'; Project = 'pipes/pipes.vbp' }
    @{ Name = 'restaum'; Project = 'restaum/source/BrainVita.vbp' }
    @{ Name = 'sumboku'; Project = 'sumboku/v1.4.2/SuMBOku_V_1_4/Sudoku.vbp' }
)

function Get-InputDirectory([string]$Architecture) {
    return Join-Path $root "$InputRoot/$Architecture"
}

function Invoke-Collect([string]$Architecture) {
    $build = Join-Path $root "out/build/windows-$Architecture-release/Release"
    if (-not (Test-Path (Join-Path $build 'wfc.exe'))) {
        throw "No Release wfc.exe for ${Architecture}: $build"
    }
    $target = Get-InputDirectory $Architecture
    if (Test-Path $target) { Remove-Item $target -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    foreach ($name in 'wfc.exe', 'wfcl.exe', 'wfcrt.dll', 'wfcvbrun.dll',
        'wfcmscomctl.dll', 'wfccomdlg32.dll', 'wfcmswinsck.dll') {
        if (Test-Path (Join-Path $build $name)) {
            Copy-Item (Join-Path $build $name) $target
        }
    }
    foreach ($game in $games) {
        $source = Join-Path $root "tests/vb6games/$($game.Project)"
        $folder = Join-Path $target "games/$($game.Name)"
        New-Item -ItemType Directory -Force -Path $folder | Out-Null
        # Both ways: one file with the whole runtime (static), and a small
        # program that loads wfcrt.dll from the installation (dynamic).
        foreach ($link in 'static', 'dynamic') {
            $suffix = if ($link -eq 'static') { '' } else { '-dynamic' }
            $program = Join-Path $folder `
                "$($game.Name)-$Architecture$suffix.exe"
            & (Join-Path $build 'wfc.exe') --build $source -o $program --gui `
                --link $link
            if ($LASTEXITCODE -ne 0) {
                throw "wfc --build ($link) failed for $($game.Name) " +
                    "($Architecture)"
            }
        }
    }
}

function Write-PackageInfo([string]$Directory, [string]$Name,
                           [string]$Description, [string]$Revision,
                           [string]$PackageVersion = $Version) {
    New-Item -ItemType Directory -Force `
        -Path (Join-Path $Directory '.wpm') | Out-Null
    @(
        "name=$Name"
        "version=$PackageVersion"
        'arch=any'
        'debug=false'
        "description=$Description"
        'maintainer=Jordan Waughtal'
        'homepage=https://github.com/Thewafflication/wfc'
        'repository=https://github.com/Thewafflication/wfc'
        'license=GPL-3.0-or-later'
        "source-version=$PackageVersion"
        "source-revision=$Revision"
    ) | Set-Content (Join-Path $Directory '.wpm/package.txt') -Encoding ascii
}

function Build-Package([string]$Directory, [string]$Name, [string]$Packages,
                       [string]$PackageVersion = $Version) {
    $arguments = @('build', $Directory, $Packages)
    if ($SigningKey) {
        $arguments += @('--sign', (Resolve-Path $SigningKey).Path)
    }
    & $Wpm @arguments
    if ($LASTEXITCODE -ne 0) { throw "wpm build failed for $Name" }
    $package = Join-Path $Packages "$Name-any-$PackageVersion.zip"
    if (-not (Test-Path $package)) { throw "Missing package: $package" }
    return $package
}

Push-Location $root
try {
    if ($Collect) {
        foreach ($architecture in $Architectures) {
            Invoke-Collect $architecture
        }
        return
    }

    if (-not $Version) { throw 'Assembling needs -Version.' }
    $revision = (git rev-parse HEAD).Trim()
    $staging = Join-Path $root 'out/wpm-staging'
    $packages = Join-Path $root $Output
    if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $staging, $packages | Out-Null

    # --- wfc -------------------------------------------------------------
    $tool = Join-Path $staging 'wfc-any'
    foreach ($architecture in $Architectures) {
        $collected = Get-InputDirectory $architecture
        if (-not (Test-Path (Join-Path $collected 'wfc.exe'))) {
            throw "Nothing collected for ${architecture}: $collected"
        }
        $target = Join-Path $tool $architecture
        New-Item -ItemType Directory -Force -Path $target | Out-Null
        foreach ($name in 'wfc.exe', 'wfcl.exe', 'wfcrt.dll', 'wfcvbrun.dll',
            'wfcmscomctl.dll', 'wfccomdlg32.dll', 'wfcmswinsck.dll') {
            if (Test-Path (Join-Path $collected $name)) {
                Copy-Item (Join-Path $collected $name) $target
            }
        }
    }
    Copy-Item LICENSE $tool
    # WPM runs .wpm/install.cmd and .wpm/remove.cmd: they hand over to the
    # install script, which installs machine-wide as an administrator (C:\wfc)
    # and for the current user otherwise.
    Copy-Item scripts/Install-Wfc.ps1, scripts/WfcInstallCommon.ps1 $tool
    New-Item -ItemType Directory -Force -Path (Join-Path $tool '.wpm') |
        Out-Null
    $script = '"%~dp0..\Install-Wfc.ps1"'
    Set-Content (Join-Path $tool '.wpm/install.cmd') -Encoding ascii -Value @(
        '@echo off'
        "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
            '-Source "%~dp0.." || exit /b 1'
        'exit /b 0'
    )
    Set-Content (Join-Path $tool '.wpm/remove.cmd') -Encoding ascii -Value @(
        '@echo off'
        "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
            '-Uninstall || exit /b 1'
        'exit /b 0'
    )
    Copy-Item README.MD $tool
    Copy-Item planning/compatibility-profile-1.0.md $tool
    Write-PackageInfo $tool 'wfc' 'Waughtal Form Compiler: a VB6 interpreter' `
        $revision
    $toolPackage = Build-Package $tool 'wfc' $packages

    # --- wfc-games -------------------------------------------------------
    $gamesRoot = Join-Path $staging 'wfc-games-any'
    $sums = New-Object System.Collections.Generic.List[string]
    foreach ($game in $games) {
        $folder = Join-Path $gamesRoot $game.Name
        New-Item -ItemType Directory -Force -Path $folder | Out-Null
        $gameRoot = "tests/vb6games/$($game.Project.Split('/')[0])"
        Copy-Item (Join-Path $root "$gameRoot/PROVENANCE.md") $folder
        foreach ($license in 'LICENSE', 'LICENSE.txt', 'LICENSE.md') {
            $path = Join-Path $root "$gameRoot/$license"
            if (Test-Path $path) { Copy-Item $path $folder }
        }
        foreach ($architecture in $Architectures) {
            foreach ($suffix in '', '-dynamic') {
                $name = "$($game.Name)-$architecture$suffix.exe"
                $built = Join-Path (Get-InputDirectory $architecture) `
                    "games/$($game.Name)/$name"
                Copy-Item $built $folder
                $hash = (Get-FileHash (Join-Path $folder $name) `
                    -Algorithm SHA256).Hash.ToLower()
                $sums.Add("$hash  $($game.Name)/$name")
            }
        }
    }
    $sums | Set-Content (Join-Path $gamesRoot 'SHA256SUMS') -Encoding ascii
    Copy-Item LICENSE (Join-Path $gamesRoot 'WFC-LICENSE')
    Write-PackageInfo $gamesRoot 'wfc-games' `
        'VB6 games from the vb6games corpus, packaged with WFC' $revision
    $gamesPackage = Build-Package $gamesRoot 'wfc-games' $packages

    # --- one package per library ---------------------------------------
    # Each library has its own version (lib/<name>/VERSION) and package, so a
    # dynamically linked program gets an update by installing that package
    # alone. The install script copies the DLL to lib\<name>\<version>\<arch>
    # of the installation and registers it with Windows.
    $libraryPackages = @()
    foreach ($library in 'vbrun', 'mscomctl', 'comdlg32', 'mswinsck') {
        $dll = "wfc$library.dll"
        $present = @($Architectures | Where-Object {
                Test-Path (Join-Path (Get-InputDirectory $_) $dll)
            })
        if ($present.Count -eq 0) { continue }
        $versionFile = "lib/wfc-$library/VERSION"
        $libraryVersion = (Get-Content $versionFile -TotalCount 1).Trim()
        $folder = Join-Path $staging "wfc-$library-any"
        foreach ($architecture in $present) {
            $target = Join-Path $folder $architecture
            New-Item -ItemType Directory -Force -Path $target | Out-Null
            $built = Join-Path (Get-InputDirectory $architecture) $dll
            Copy-Item $built $target
        }
        Copy-Item LICENSE $folder
        Copy-Item "lib/wfc-$library/README.md" $folder
        Copy-Item scripts/Install-WfcLibrary.ps1, scripts/WfcInstallCommon.ps1 `
            $folder
        New-Item -ItemType Directory -Force -Path (Join-Path $folder '.wpm') |
            Out-Null
        $script = '"%~dp0..\Install-WfcLibrary.ps1"'
        $install = Join-Path $folder '.wpm/install.cmd'
        Set-Content $install -Encoding ascii -Value @(
            '@echo off'
            "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
                "-Source `"%~dp0..`" -Library wfc$library || exit /b 1"
            'exit /b 0'
        )
        $remove = Join-Path $folder '.wpm/remove.cmd'
        Set-Content $remove -Encoding ascii -Value @(
            '@echo off'
            "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
                "-Source `"%~dp0..`" -Library wfc$library -Uninstall " +
                '|| exit /b 1'
            'exit /b 0'
        )
        Write-PackageInfo $folder "wfc-$library" `
            "WFC $library library $libraryVersion (registered with Windows)" `
            $revision $libraryVersion
        $libraryPackages += Build-Package $folder "wfc-$library" $packages `
            $libraryVersion
    }

    $toolPackage
    $gamesPackage
    $libraryPackages
}
finally {
    Pop-Location
}
