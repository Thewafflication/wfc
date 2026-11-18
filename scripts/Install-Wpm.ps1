<#
.SYNOPSIS
Installs the pinned WPM release on a CI runner and puts it on the PATH.

.DESCRIPTION
Downloads wpm-x64-<version>.zip from the WPM releases, checks it against the
pinned SHA-256 digest, runs its setup and adds the WPM folder to GITHUB_PATH
when that is set. Used by the package and verify jobs of the workflow.

.PARAMETER Version
WPM version; the digest below belongs to it.

.PARAMETER Sha256
SHA-256 digest of the archive.
#>
[CmdletBinding()]
param(
    [string]$Version = '1.5.1',
    [string]$Sha256 = ('a701899e42561bdb659e4a8a4881b2d4' +
        'e5dd220133a443ae0e14c194b60f915d')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$temp = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else {
    [IO.Path]::GetTempPath()
}
$archive = Join-Path $temp "wpm-x64-$Version.zip"
$base = 'https://github.com/Thewafflication/wpm/releases/download'
Invoke-WebRequest -UseBasicParsing "$base/$Version/wpm-x64-$Version.zip" `
    -OutFile $archive
$actual = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
if ($actual -ne $Sha256) {
    throw 'Pinned WPM archive digest mismatch.'
}
$package = Join-Path $temp 'wpm-package'
if (Test-Path $package) { Remove-Item $package -Recurse -Force }
Expand-Archive -LiteralPath $archive -DestinationPath $package
& (Join-Path $package 'setup.cmd') --machine (Join-Path $package 'wpm.exe')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$wpmDirectory = Join-Path $env:ProgramFiles 'WPM'
$wpmExe = Join-Path $wpmDirectory 'wpm.exe'
$reported = (& $wpmExe --version 2>&1) -join "`n"
if ($reported -notmatch "Version $([regex]::Escape($Version))") {
    throw "Unexpected WPM version: $reported"
}
if ($env:GITHUB_PATH) {
    $wpmDirectory | Out-File -Append -Encoding utf8 $env:GITHUB_PATH
}
