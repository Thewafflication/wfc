<#
.SYNOPSIS
Runs the WFC canonical build stage.
.DESCRIPTION
Configures and builds one CMake preset. The local commit gate uses the
x64 Debug preset; CI passes each matrix preset.
.PARAMETER Preset
CMake configure and build preset name.
.EXAMPLE
pwsh -NoProfile -File scripts/Invoke-Build.ps1
.EXAMPLE
pwsh -NoProfile -File scripts/Invoke-Build.ps1 -Preset windows-arm64-debug
#>
[CmdletBinding()]
param(
    [string]$Preset = 'windows-x64-debug'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'WfcChecks.psm1') -Force

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    Invoke-CheckedCommand cmake @('--preset', $Preset)
    Invoke-CheckedCommand cmake @('--build', '--preset', $Preset)
    Write-Output "[PASS] build $Preset"
}
finally {
    Pop-Location
}
