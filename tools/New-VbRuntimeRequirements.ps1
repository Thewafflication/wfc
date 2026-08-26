[CmdletBinding()]
param(
    [Parameter()]
    [string] $InventoryPath = '.\evidence\reference\msvbvm60-6.0-runtime.json',

    [Parameter()]
    [string] $OutputDirectory = '.\requirements\vbrun'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Add-Header {
    param($Lines, [string] $Id, [string] $Title)
    $Lines.Add("# $Id — $Title")
    $Lines.Add('')
    $Lines.Add('**Content type:** Project requirement')
    $Lines.Add('')
    $Lines.Add('**Status:** Proposed')
    $Lines.Add('')
    $Lines.Add('**Source:** Installed Visual Basic 6.0 runtime (`MSVBVM60.DLL`)')
    $Lines.Add('')
}

function Add-Footer {
    param($Lines, [string] $Verification)
    $Lines.Add('## Rationale')
    $Lines.Add('')
    $Lines.Add('Native-code VB6 programs import runtime entry points directly. Their')
    $Lines.Add('names, ordinals, calling conventions, data layouts, and behavior therefore')
    $Lines.Add('form a binary compatibility boundary independent of the VBA object model.')
    $Lines.Add('')
    $Lines.Add('## Verification')
    $Lines.Add('')
    $Lines.Add('**Method:** Inspection and test')
    $Lines.Add('')
    $Lines.Add('**References:** `evidence/reference/msvbvm60-6.0-runtime.json`; test case TBD')
    $Lines.Add('')
    $Lines.Add($Verification)
    $Lines.Add('')
    $Lines.Add('## Relationships')
    $Lines.Add('')
    $Lines.Add('- **Derived from:** MP-0001 compatibility discovery objective')
    $Lines.Add('- **Depends on:** WCRT integration and VB execution-engine architecture')
    $Lines.Add('- **Conflicts with:** None known')
    $Lines.Add('')
    $Lines.Add('## Tailoring')
    $Lines.Add('')
    $Lines.Add('An export or behavior may be deferred only through an approved compatibility')
    $Lines.Add('profile that identifies the affected compiled VB6 programs.')
    $Lines.Add('')
    $Lines.Add('## Implementation Record')
    $Lines.Add('')
    $Lines.Add('Not implemented.')
}

function Add-ExportTable {
    param($Lines, $Exports)
    $Lines.Add('| Ordinal | Export | Reference RVA |')
    $Lines.Add('| ---: | --- | ---: |')
    foreach ($export in @($Exports | Sort-Object ordinal)) {
        $lines.Add("| $($export.ordinal) | ``$($export.name)`` | ``$($export.rva)`` |")
    }
    $Lines.Add('')
}

$inventory = Get-Content -LiteralPath $InventoryPath -Raw | ConvertFrom-Json
$resolvedOutput = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($resolvedOutput) | Out-Null
$exports = @($inventory.exportDirectory.exports | Where-Object present)

$lines = [System.Collections.Generic.List[string]]::new()
Add-Header $lines 'REQ-0099' 'VB6 runtime binary identity and loader contract'
$lines.Add('## Requirement')
$lines.Add('')
$lines.Add('The WFC VB6 runtime compatibility component shall provide an x86 PE DLL that')
$lines.Add('can satisfy references to `MSVBVM60.DLL`, load in the Windows GUI subsystem,')
$lines.Add('and provide the export ABI required by native VB6 executables. Cross-architecture')
$lines.Add('WFC hosts shall isolate or bridge this x86 ABI explicitly rather than loading it')
$lines.Add('in-process on x64 or ARM64.')
$lines.Add('')
$lines.Add('## Reference Binary')
$lines.Add('')
$lines.Add('| Item | Observed value |')
$lines.Add('| --- | --- |')
$lines.Add("| SHA-256 | ``$($inventory.source.sha256)`` |")
$lines.Add("| File/product version | ``$($inventory.source.fileVersion)`` / ``$($inventory.source.productVersion)`` |")
$lines.Add("| Size | $($inventory.source.size) bytes |")
$lines.Add("| PE machine | ``$($inventory.pe.machine)`` ($($inventory.pe.architecture)) |")
$lines.Add("| Image base | ``$($inventory.pe.imageBase)`` |")
$lines.Add("| Subsystem | $($inventory.pe.subsystem) (Windows GUI) |")
$lines.Add("| Authenticode status | $($inventory.source.signatureStatus) |")
$lines.Add('')
$lines.Add('The observed imports are: ' + (($inventory.pe.dependencies | ForEach-Object { "``$_``" }) -join ', ') + '.')
$lines.Add('')
Add-Footer $lines 'Build and load the compatibility DLL from a minimal x86 native executable, inspect its PE headers, and verify that supported x64 and ARM64 hosts use the selected compatibility bridge.'
[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'req-0099-runtime-binary-loader-contract.md'), $lines, [Text.UTF8Encoding]::new($false))

$groups = @(
    [ordered]@{ id='REQ-0100'; category='engine-and-hosting'; file='req-0100-engine-hosting-exports.md'; title='VB6 execution-engine and hosting exports'; behavior='WFC shall reproduce project and context loading, native EXE/DLL startup, COM class-factory and registration entry points, type-instance management, Automation dispatch, event-sink dispatch, and teardown semantics represented by these exports.' },
    [ordered]@{ id='REQ-0101'; category='compiler-helper'; file='req-0101-compiler-helper-exports.md'; title='VB6 compiler-helper export ABI'; behavior='WFC shall reproduce the compiler helper ABI used for VB values, arrays, strings, objects, variants, comparisons, arithmetic, conversions, loops, exceptions, errors, file statements, late binding, and procedure cleanup.' },
    [ordered]@{ id='REQ-0102'; category='language-runtime'; file='req-0102-language-runtime-exports.md'; title='VB6 language-runtime service exports'; behavior='WFC shall reproduce the callable language services used for strings, characters, dates and times, numeric and financial functions, formatting, conversion, filesystem and settings access, interaction, locale, random numbers, and array-producing functions.' },
    [ordered]@{ id='REQ-0103'; category='member-and-memory'; file='req-0103-member-memory-exports.md'; title='VB6 member and memory helper exports'; behavior='WFC shall reproduce compiled field access for scalar, string, variant, object, event, and newly-created object fields, together with record copy and address-observation behavior.' },
    [ordered]@{ id='REQ-0104'; category='numeric-support'; file='req-0104-numeric-support-exports.md'; title='VB6 native numeric support exports'; behavior='WFC shall reproduce the x86 floating-point adjustment, transcendental, square-root, and wide-multiply helper ABI with reference precision, rounding, exception, and stack behavior.' }
)

foreach ($group in $groups) {
    $selected = @($exports | Where-Object category -eq $group.category)
    $lines = [System.Collections.Generic.List[string]]::new()
    Add-Header $lines $group.id $group.title
    $lines.Add('## Requirement')
    $lines.Add('')
    $lines.Add($group.behavior)
    $lines.Add('')
    $lines.Add("The compatibility DLL shall export all $($selected.Count) symbols below at the")
    $lines.Add('listed ordinals. Reference RVAs identify the inspected binary but are not')
    $lines.Add('required implementation addresses.')
    $lines.Add('')
    Add-ExportTable $lines $selected
    Add-Footer $lines 'Link representative VB6 native executables that import every listed helper family, compare import resolution by name and ordinal, and compare observable results, errors, state changes, and cleanup against the reference runtime.'
    [IO.File]::WriteAllLines((Join-Path $resolvedOutput $group.file), $lines, [Text.UTF8Encoding]::new($false))
}

$lines = [System.Collections.Generic.List[string]]::new()
Add-Header $lines 'REQ-0105' 'VB6 runtime export-directory layout'
$lines.Add('## Requirement')
$lines.Add('')
$lines.Add("The compatibility DLL shall use ordinal base $($inventory.exportDirectory.ordinalBase) and")
$lines.Add("preserve the reference mapping of $($inventory.exportDirectory.namedExports) named exports within")
$lines.Add("$($inventory.exportDirectory.functionSlots) export-address-table slots. Slots that are empty in the")
$lines.Add('reference shall not resolve to an implementation. No symbol may silently move to a')
$lines.Add('different ordinal, even when normal linking uses its name.')
$lines.Add('')
$lines.Add('The complete ordinal-to-name, RVA, presence, and category mapping is retained in')
$lines.Add('`evidence/reference/msvbvm60-6.0-runtime.json`.')
$lines.Add('')
Add-Footer $lines 'Parse the built PE export directory and compare ordinal base, table size, holes, names, and ordinal assignments with the retained machine-readable inventory.'
[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'req-0105-export-directory-layout.md'), $lines, [Text.UTF8Encoding]::new($false))

$counts = $inventory.exportDirectory.categoryCounts
$index = @(
    '# Visual Basic 6.0 Runtime Requirements',
    '',
    '**Content type:** Project requirement index',
    '',
    '**Status:** Proposed',
    '',
    '**Reference baseline:** Installed `MSVBVM60.DLL` 6.00.9848 (x86)',
    '',
    '| Requirement | Contract | Export count |',
    '| --- | --- | ---: |',
    '| `REQ-0099` | [Binary identity and loader](req-0099-runtime-binary-loader-contract.md) | — |',
    "| ``REQ-0100`` | [Execution engine and hosting](req-0100-engine-hosting-exports.md) | $($counts.'engine-and-hosting') |",
    "| ``REQ-0101`` | [Compiler helpers](req-0101-compiler-helper-exports.md) | $($counts.'compiler-helper') |",
    "| ``REQ-0102`` | [Language-runtime services](req-0102-language-runtime-exports.md) | $($counts.'language-runtime') |",
    "| ``REQ-0103`` | [Member and memory helpers](req-0103-member-memory-exports.md) | $($counts.'member-and-memory') |",
    "| ``REQ-0104`` | [Native numeric support](req-0104-numeric-support-exports.md) | $($counts.'numeric-support') |",
    '| `REQ-0105` | [Export-directory layout](req-0105-export-directory-layout.md) | 635 named |',
    '',
    'The retained export directory contains 1,911 slots beginning at ordinal 100;',
    '635 slots are populated and named. Function signatures and calling conventions',
    'are not encoded by PE exports and require compiled reference probes and call-site',
    'inspection before implementation.'
)
[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'README.md'), $index, [Text.UTF8Encoding]::new($false))
