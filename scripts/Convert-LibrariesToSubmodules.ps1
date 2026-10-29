<#
.SYNOPSIS
Turns lib/wfc-vbrun and lib/wfc-ocx into Git submodules.

.DESCRIPTION
For each library: splits its history out of this repository (git subtree
split), pushes it to the library's own repository, replaces the folder with a
submodule of that repository and stages the result. Nothing is committed.

Create the two repositories first (empty, under the same owner as this one);
the script pushes to them. Run it from a clean checkout of the integration
branch.

.PARAMETER Owner
The GitHub owner of the new repositories.

.PARAMETER Libraries
The libraries to convert; each lives in lib/<name> and goes to the repository
<Owner>/<name>.

.PARAMETER WhatIf
Shows what would be done.

.EXAMPLE
pwsh -NoProfile `
  -File scripts/Convert-LibrariesToSubmodules.ps1 -Owner Thewafflication
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][string]$Owner,
    [string[]]$Libraries = @('wfc-vbrun', 'wfc-ocx')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Git {
    & git @args
    if ($LASTEXITCODE -ne 0) {
        throw "git $($args -join ' ') failed ($LASTEXITCODE)"
    }
}

if (git status --porcelain) {
    throw 'The working tree is not clean.'
}
foreach ($name in $Libraries) {
    $path = "lib/$name"
    $remote = "https://github.com/$Owner/$name.git"
    if (-not (Test-Path -LiteralPath $path)) {
        throw "$path does not exist (already a submodule?)."
    }
    $action = "Split, push to $remote, and add as a submodule"
    if (-not $PSCmdlet.ShouldProcess($path, $action)) {
        continue
    }
    $branch = "split/$name"
    Invoke-Git subtree split --prefix=$path -b $branch
    Invoke-Git push $remote "${branch}:refs/heads/master"
    Invoke-Git rm -r -q $path
    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Recurse -Force
    }
    Invoke-Git submodule add $remote $path
    Invoke-Git branch -D $branch
}
Write-Output 'Review with git status, then commit; CI needs submodule checkout.'
