<#
.SYNOPSIS
Pre-push hook stage: runs the full test suite when a tag is pushed.
.DESCRIPTION
A branch push does nothing (the commit gate already ran the tests of the
changed modules and CI runs the full suite). Pushing a tag is a release: every
test of wfc and of all its submodules must pass, so this runs
scripts/Invoke-Tests.ps1 with the Full scope.
pre-commit names the pushed ref in PRE_COMMIT_REMOTE_BRANCH.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$remoteRef = $env:PRE_COMMIT_REMOTE_BRANCH
if (-not $remoteRef -or -not $remoteRef.StartsWith('refs/tags/')) {
    Write-Output '[tests] not a tag push: full suite skipped'
    return
}
Write-Output "[tests] release tag $remoteRef: full suite"
& pwsh -NoProfile -File (Join-Path $PSScriptRoot 'Invoke-Tests.ps1') -Scope Full
if ($LASTEXITCODE -ne 0) {
    throw "Full test suite failed ($LASTEXITCODE); the tag was not pushed."
}
