<#
.SYNOPSIS
Verifies a folder of signed WPM release packages before they are published.

.DESCRIPTION
Checks that SHA256SUMS lists every package and index.json with the right
digests, that index.json names each package once with its name, architecture
and version, and that wpm verify accepts the signature of each package once
the release key is trusted. Fails on the first problem.

.PARAMETER Directory
The folder with the packages, index.json and SHA256SUMS.

.PARAMETER Count
How many packages the folder must hold.

.PARAMETER PublicKey
The release public key to trust.

.PARAMETER Wpm
Path of wpm.exe.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Directory,
    [Parameter(Mandatory)][int]$Count,
    [string]$PublicKey = (Join-Path (Split-Path -Parent $PSScriptRoot) `
            'release_keys/wfc-release.public'),
    [string]$Wpm = 'wpm.exe'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$packages = @(Get-ChildItem -LiteralPath $Directory -Filter *.zip -File |
        Sort-Object Name)
if ($packages.Count -ne $Count) {
    throw "Expected $Count packages, found $($packages.Count)."
}

# --- checksums -----------------------------------------------------------
$sumsPath = Join-Path $Directory 'SHA256SUMS'
$sums = @{}
foreach ($line in Get-Content -LiteralPath $sumsPath) {
    if ($line -notmatch '^(?<hash>[0-9a-f]{64})  (?<name>\S+)$') {
        throw "Malformed SHA256SUMS line: $line"
    }
    $sums[$Matches.name] = $Matches.hash
}
$expected = @($packages.Name) + 'index.json'
if ($sums.Count -ne $expected.Count) {
    throw "SHA256SUMS lists $($sums.Count) files, expected $($expected.Count)."
}
foreach ($name in $expected) {
    if (-not $sums.ContainsKey($name)) {
        throw "SHA256SUMS does not list $name."
    }
    $path = Join-Path $Directory $name
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    if ($actual.ToLowerInvariant() -ne $sums[$name]) {
        throw "Checksum mismatch: $name"
    }
    Write-Output "checksum ok  $name"
}

# --- index ---------------------------------------------------------------
$index = Get-Content -LiteralPath (Join-Path $Directory 'index.json') -Raw |
    ConvertFrom-Json
$entries = @($index.packages)
if ($entries.Count -ne $Count) {
    throw "index.json names $($entries.Count) packages, expected $Count."
}
foreach ($entry in $entries) {
    $file = "$($entry.name)-$($entry.arch)-$($entry.version).zip"
    if ($packages.Name -notcontains $file) {
        throw "index.json names $file, which is not in the folder."
    }
    if (-not $entry.url.EndsWith("/$file")) {
        throw "index.json gives $file the address $($entry.url)."
    }
}
Write-Output "index ok     $($entries.Count) packages"

# --- signatures ----------------------------------------------------------
& $Wpm trust add $PublicKey
if ($LASTEXITCODE -ne 0) { throw "wpm trust add failed ($LASTEXITCODE)." }
foreach ($package in $packages) {
    & $Wpm verify $package.FullName
    if ($LASTEXITCODE -ne 0) {
        throw "wpm verify failed for $($package.Name)."
    }
}
Write-Output "verified     $($packages.Count) packages"
