<#
.SYNOPSIS
Runs the WFC canonical full test stage.
.DESCRIPTION
Runs every registered CTest test for one preset, including negative tests.
Zero registered tests is an error. The preset must already be built
(scripts/Invoke-Build.ps1).
.PARAMETER Preset
CMake test preset name.
.PARAMETER Scope
Full (the default, used by CI and releases) runs every registered test.
Changed (the commit gate) runs the tests of the modules the staged files
belong to (scripts/test-modules.json); files that belong to no module run the
fast tier, which is every test except the slow probes and game packages.
Docs-only changes run nothing.
.PARAMETER JUnitPath
Optional path for a JUnit XML result file (used by CI evidence).
.EXAMPLE
pwsh -NoProfile -File scripts/Invoke-Tests.ps1
#>
[CmdletBinding()]
param(
    [string]$Preset = 'windows-x64-debug',
    [ValidateSet('Full', 'Changed')]
    [string]$Scope = 'Full',
    [string]$JUnitPath = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'WfcChecks.psm1') -Force

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    # The log is followed live by scripts/Show-TestProgress.ps1.
    $progressLog = Join-Path $root 'out/test-progress.log'
    New-Item -ItemType Directory -Force -Path (Split-Path $progressLog) |
        Out-Null
    $arguments = @('--preset', $Preset, '--output-on-failure')
    if ($Scope -eq 'Full') {
        $arguments += '--no-tests=error'
    } else {
        $selection = Get-ChangedTestSelection -Root $root
        Write-Output "[tests] scope Changed: $($selection.Reason)"
        if ($selection.Skip) {
            Write-Output "[PASS] tests $Preset (nothing to run)"
            return
        }
        $arguments += $selection.Arguments
    }
    if ($JUnitPath) {
        $arguments += @('--output-junit', $JUnitPath)
    }
    # Each line goes to the log as soon as ctest prints it.
    [IO.File]::WriteAllText($progressLog, '')
    & ctest @arguments 2>&1 | ForEach-Object {
        $line = "$_"
        [IO.File]::AppendAllText($progressLog, $line + "`n")
        $line
    }
    if ($LASTEXITCODE -ne 0) {
        throw "ctest $($arguments -join ' ') failed ($LASTEXITCODE)."
    }
    Write-Output "[PASS] tests $Preset"
}
finally {
    Pop-Location
}
