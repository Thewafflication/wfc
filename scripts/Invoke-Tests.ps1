<#
.SYNOPSIS
Runs the WFC canonical full test stage.
.DESCRIPTION
Runs every registered CTest test for one preset, including negative tests.
Zero registered tests is an error. The preset must already be built
(scripts/Invoke-Build.ps1).
.PARAMETER Preset
CMake test preset name.
.PARAMETER JUnitPath
Optional path for a JUnit XML result file (used by CI evidence).
.EXAMPLE
pwsh -NoProfile -File scripts/Invoke-Tests.ps1
#>
[CmdletBinding()]
param(
    [string]$Preset = 'windows-x64-debug',
    [string]$JUnitPath = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'WfcChecks.psm1') -Force

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $arguments = @(
        '--preset', $Preset, '--output-on-failure', '--no-tests=error')
    if ($JUnitPath) {
        $arguments += @('--output-junit', $JUnitPath)
    }
    Invoke-CheckedCommand ctest $arguments
    Write-Output "[PASS] tests $Preset"
}
finally {
    Pop-Location
}
