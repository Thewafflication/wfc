<#
.SYNOPSIS
Runs the WFC canonical lint stage.
.DESCRIPTION
Checks the complete owned source and configuration scope recorded in
planning/commit-checks.md:
- WSP physical style (80 columns, UTF-8, whitespace, final newline);
- clang-format check mode for C++ (WSP-LANG-0002);
- gersemi check mode for CMake (WSP-LANG-0010);
- PowerShell syntax and PSScriptAnalyzer (WSP-LANG-0007);
- JSON and YAML syntax (WSP-LANG-0004, WSP-LANG-0005);
- pre-commit configuration validity (WSP-CHECK-0001).
Stops at the first failing check and exits nonzero. Changes no files.
.EXAMPLE
pwsh -NoProfile -File scripts/Invoke-Lint.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'WfcChecks.psm1') -Force

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    Write-Output '[lint] WSP physical source style'
    $styleScope = @(
        'src', 'include', 'tests', 'cmake', 'scripts', '.github',
        'CMakeLists.txt', 'CMakePresets.json', '.clang-format',
        '.editorconfig', '.gersemirc', '.gitattributes',
        '.pre-commit-config.yaml', 'PSScriptAnalyzerSettings.psd1'
    )
    # A child process gives a reliable exit status; -Command keeps the
    # array parameter intact.
    $quoted = ($styleScope | ForEach-Object { "'$_'" }) -join ','
    Invoke-CheckedCommand pwsh @('-NoProfile', '-Command',
        "& ./wsp/tools/Test-SourceStyle.ps1 -RepositoryRoot '$root' " +
        "-SourcePath @($quoted)")

    Write-Output '[lint] clang-format (C++)'
    $clangFormat = Resolve-PinnedTool 'clang-format' '22.1.3'
    $cxx = Get-OwnedFile @('src', 'include', 'tests') @('.cpp', '.hpp')
    Invoke-CheckedCommand $clangFormat (
        @('--dry-run', '--Werror', '--style=file') + $cxx)

    Write-Output '[lint] gersemi (CMake)'
    $gersemi = Resolve-PinnedTool 'gersemi' '0.29.2'
    $cmake = @('CMakeLists.txt') +
        (Get-OwnedFile @('cmake', 'tests') @('.cmake'))
    # Definitions teach gersemi the project's own functions, so an unknown
    # command is a real finding under --warnings-as-errors.
    Invoke-CheckedCommand $gersemi (@(
            '--check', '--warnings-as-errors',
            '--definitions', 'cmake', 'tests', '--') + $cmake)

    Write-Output '[lint] PowerShell syntax and analysis'
    & ./wsp/tools/Test-RepositorySyntax.ps1
    Import-Module PSScriptAnalyzer -RequiredVersion 1.25.0
    $findings = @(Invoke-ScriptAnalyzer -Path scripts -Recurse `
            -Settings PSScriptAnalyzerSettings.psd1)
    if ($findings.Count -gt 0) {
        $findings | Format-Table ScriptName, Line, RuleName, Message -Wrap |
            Out-String | Write-Output
        throw 'PowerShell analysis failed; see findings above.'
    }

    Write-Output '[lint] JSON and YAML syntax'
    foreach ($file in Get-OwnedFile @('CMakePresets.json') @('.json')) {
        Get-Content -LiteralPath $file -Raw | ConvertFrom-Json | Out-Null
    }
    $yaml = Get-OwnedFile @('.github', '.pre-commit-config.yaml') `
        @('.yml', '.yaml')
    $parseYaml = 'import sys, yaml; [yaml.safe_load(open(p, ' +
        "encoding='utf-8')) for p in sys.argv[1:]]"
    Invoke-CheckedCommand python (@('-c', $parseYaml) + $yaml)

    Write-Output '[lint] pre-commit configuration'
    $preCommit = Resolve-PinnedTool 'pre-commit' '4.6.2'
    Invoke-CheckedCommand $preCommit @('validate-config')

    Write-Output '[PASS] lint'
}
finally {
    Pop-Location
}
