<#
.SYNOPSIS
Builds the WFC WPM packages: one per Windows architecture and one for Linux.

.DESCRIPTION
Two stages (see planning/MP-0008-release-plan.md):

  -Collect   Run on each architecture's own machine: copies that
             architecture's Release wfc.exe, wfcl.exe, wfcrt.dll and the
             library DLLs into out/package-input/<arch>.

  (default)  Assembles from out/package-input:
               wfc-<arch>-<version>.zip    for each of -Architectures (x86,
                                           x64, arm64): wfc.exe, wfcl.exe,
                                           wfcw2k.exe and wfcrt.dll in an
                                           <arch> folder, LICENSE, README.MD,
                                           the install scripts; it depends on
                                           the library packages (wfc-vbrun,
                                           wfc-mscomctl, wfc-mscomctl2,
                                           wfc-comdlg32, wfc-mswinsck) at the
                                           versions in lib/*/VERSION
               wfc-linux-any-<version>.zip for -LinuxArchitectures, from
                                           out/package-input/linux-<arch>
                                           (the Winelib build: wfc.exe.so and
                                           its launcher), LICENSE, README.MD

The corpus games and the libraries are packaged and released by their own
repositories (vb6games, wfc-vbrun, wfc-mscomctl, ...).

Nothing is signed unless -SigningKey is given (CI passes the key from the
protected release environment), and nothing is published.

.PARAMETER Version
Package version, for example 0.8.0.

.PARAMETER Collect
Collect stage for one architecture (see -Architectures).

.PARAMETER Architectures
Windows architectures to collect or to package; Collect needs the Release
build (out/build/windows-<arch>-release/Release).

.PARAMETER LinuxArchitectures
Linux architectures for wfc-linux-any; none by default.

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
    [string[]]$LinuxArchitectures = @(),
    [string]$Wpm = 'wpm.exe',
    [string]$SigningKey,
    [string]$InputRoot = 'out/package-input',
    [string]$Output = 'out/packages'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path -Parent $PSScriptRoot
$toolFiles = @(
    'wfc.exe', 'wfcl.exe', 'wfcw2k.exe', 'wfcrt.dll', 'msvbvm60.dll',
    'mscomctl.ocx', 'mscomct2.ocx', 'comdlg32.ocx', 'mswinsck.ocx'
)

# What a wfc package carries; the library DLLs come in their own packages.
$packageFiles = @('wfc.exe', 'wfcl.exe', 'wfcw2k.exe', 'wfcrt.dll')
$libraries = 'vbrun', 'mscomctl', 'mscomctl2', 'comdlg32', 'mswinsck'

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
    foreach ($name in $toolFiles) {
        if (Test-Path (Join-Path $build $name)) {
            Copy-Item (Join-Path $build $name) $target
        }
    }
}

function Write-PackageInfo([string]$Directory, [string]$Name,
                           [string]$Description, [string]$Revision,
                           [string]$Architecture = 'any',
                           [string[]]$Depends = @()) {
    New-Item -ItemType Directory -Force `
        -Path (Join-Path $Directory '.wpm') | Out-Null
    @(
        "name=$Name"
        "version=$Version"
        "arch=$Architecture"
        'debug=false'
        "description=$Description"
        'maintainer=Jordan Waughtal'
        'homepage=https://github.com/Thewafflication/wfc'
        'repository=https://github.com/Thewafflication/wfc'
        'license=GPL-3.0-or-later'
        "source-version=$Version"
        "source-revision=$Revision"
        foreach ($dependency in $Depends) { "depends=$dependency" }
    ) | Set-Content (Join-Path $Directory '.wpm/package.txt') -Encoding ascii
}

function Build-Package([string]$Directory, [string]$Name, [string]$Packages,
                       [string]$Architecture = 'any') {
    $arguments = @('build', $Directory, $Packages)
    if ($SigningKey) {
        $arguments += @('--sign', (Resolve-Path $SigningKey).Path)
    }
    & $Wpm @arguments
    if ($LASTEXITCODE -ne 0) { throw "wpm build failed for $Name" }
    $package = Join-Path $Packages "$Name-$Architecture-$Version.zip"
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

    # Each library at the version this source carries (its submodule).
    $depends = foreach ($library in $libraries) {
        $libraryVersion = (Get-Content "lib/wfc-$library/VERSION" `
                -TotalCount 1).Trim()
        "wfc-$library>=$libraryVersion"
    }

    # --- wfc, one package per Windows architecture ----------------------
    foreach ($architecture in $Architectures) {
        $collected = Get-InputDirectory $architecture
        if (-not (Test-Path (Join-Path $collected 'wfc.exe'))) {
            throw "Nothing collected for ${architecture}: $collected"
        }
        $tool = Join-Path $staging "wfc-$architecture"
        $target = Join-Path $tool $architecture
        New-Item -ItemType Directory -Force -Path $target | Out-Null
        foreach ($name in $packageFiles) {
            if (Test-Path (Join-Path $collected $name)) {
                Copy-Item (Join-Path $collected $name) $target
            }
        }
        Copy-Item LICENSE $tool
        # WPM runs .wpm/install.cmd and .wpm/remove.cmd: they hand over to the
        # install script, which installs machine-wide as an administrator
        # (C:\wfc) and for the current user otherwise.
        Copy-Item scripts/Install-Wfc.ps1, scripts/WfcInstallCommon.ps1 $tool
        New-Item -ItemType Directory -Force -Path (Join-Path $tool '.wpm') |
            Out-Null
        $script = '"%~dp0..\Install-Wfc.ps1"'
        Set-Content (Join-Path $tool '.wpm/install.cmd') -Encoding ascii `
            -Value @(
            '@echo off'
            "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
                '-Source "%~dp0.." || exit /b 1'
            'exit /b 0'
        )
        Set-Content (Join-Path $tool '.wpm/remove.cmd') -Encoding ascii `
            -Value @(
            '@echo off'
            "powershell -NoProfile -ExecutionPolicy Bypass -File $script " +
                '-Uninstall || exit /b 1'
            'exit /b 0'
        )
        Copy-Item README.MD $tool
        Copy-Item planning/compatibility-profile-1.0.md $tool
        Write-PackageInfo $tool 'wfc' `
            "Waughtal Form Compiler: a VB6 interpreter ($architecture)" `
            $revision $architecture $depends
        Build-Package $tool 'wfc' $packages $architecture
    }

    # --- wfc-linux, the Winelib build for each Linux architecture -------
    if ($LinuxArchitectures.Count -gt 0) {
        $linux = Join-Path $staging 'wfc-linux-any'
        foreach ($architecture in $LinuxArchitectures) {
            $collected = Get-InputDirectory "linux-$architecture"
            if (-not (Test-Path (Join-Path $collected 'wfc.exe.so'))) {
                throw "Nothing collected for Linux ${architecture}: $collected"
            }
            $target = Join-Path $linux $architecture
            New-Item -ItemType Directory -Force -Path $target | Out-Null
            Copy-Item (Join-Path $collected '*') $target -Recurse
        }
        Copy-Item LICENSE, README.MD $linux
        Copy-Item planning/compatibility-profile-1.0.md $linux
        Set-Content (Join-Path $linux 'LINUX.txt') -Encoding ascii -Value @(
            'WFC for Linux, built with Wine''s winegcc (Winelib).'
            ''
            'Each folder is one architecture. Copy it anywhere, install Wine'
            '(the wine64 or wine package of the distribution) and run'
            ''
            '    wine64 <folder>/wfc.exe.so --help'
            ''
            'or the launcher script <folder>/wfc.exe when wine is on the PATH.'
            'Paths given to wfc are Linux paths.'
        )
        New-Item -ItemType Directory -Force -Path (Join-Path $linux '.wpm') |
            Out-Null
        # The package is for Linux: on Windows WPM only unpacks it.
        foreach ($name in 'install.cmd', 'remove.cmd') {
            $file = Join-Path $linux ".wpm/$name"
            Set-Content $file -Encoding ascii -Value @(
                '@echo off'
                'echo wfc-linux holds the Linux programs: see LINUX.txt.'
                'exit /b 0'
            )
        }
        Write-PackageInfo $linux 'wfc-linux' `
            'Waughtal Form Compiler for Linux (Winelib)' $revision
        Build-Package $linux 'wfc-linux' $packages
    }
}
finally {
    Pop-Location
}
