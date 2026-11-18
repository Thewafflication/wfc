<#
.SYNOPSIS
Writes index.json and SHA256SUMS for a folder of WPM release packages.

.DESCRIPTION
index.json is the WPM repository index of the release: one entry per
<name>-<arch>-<version>.zip with its download address below
<repository>/releases/download/<tag>. SHA256SUMS lists the packages and
index.json. Used by the release jobs of WFC, wfc-vbrun and vb6games.

.PARAMETER Directory
The folder with the packages.

.PARAMETER Tag
The release tag the packages are published under.

.PARAMETER Repository
The repository address; GITHUB_SERVER_URL/GITHUB_REPOSITORY by default.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Directory,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Repository = "$env:GITHUB_SERVER_URL/$env:GITHUB_REPOSITORY"
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$base = "$Repository/releases/download/$Tag"
$archives = @(Get-ChildItem -LiteralPath $Directory -Filter *.zip -File |
        Sort-Object Name)
if ($archives.Count -eq 0) { throw "No packages in $Directory." }
$pattern = '^(?<name>.+)-(?<arch>any|x86|x64|arm64)-(?<version>[^/]+)\.zip$'
$packages = foreach ($archive in $archives) {
    if ($archive.Name -notmatch $pattern) {
        throw "Unexpected WPM package filename: $($archive.Name)"
    }
    [ordered]@{
        name = $Matches.name
        version = $Matches.version
        arch = $Matches.arch
        url = "$base/$($archive.Name)"
    }
}
$index = Join-Path $Directory 'index.json'
[ordered]@{ version = 1; packages = @($packages) } |
    ConvertTo-Json -Depth 4 | Set-Content $index -Encoding utf8NoBOM
$files = @($archives.FullName) + $index
Get-FileHash -LiteralPath $files -Algorithm SHA256 |
    ForEach-Object {
        $name = Split-Path $_.Path -Leaf
        '{0}  {1}' -f $_.Hash.ToLowerInvariant(), $name
    } | Set-Content (Join-Path $Directory 'SHA256SUMS') -Encoding ascii
