[CmdletBinding()]
param(
    [Parameter()]
    [string] $InventoryPath = '.\evidence\reference\stdole-2.0-type-library.json',

    [Parameter()]
    [string] $OutputDirectory = '.\requirements\stdole'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Escape-Markdown {
    param([AllowEmptyString()][string] $Text)
    if ($null -eq $Text) { return '' }
    return $Text.Replace('|', '\|').Replace("`r", '').Replace("`n", ' ')
}

function Get-ParameterText {
    param($Parameters)
    $parts = [System.Collections.Generic.List[string]]::new()
    foreach ($parameter in @($Parameters)) {
        $direction = if (([int] $parameter.flags -band 2) -ne 0) { 'out ' } else { '' }
        $optional = if ([bool] $parameter.optional) { ' optional' } else { '' }
        $default = if ([bool] $parameter.hasDefault) { ' = ' + [string] $parameter.defaultValue } else { '' }
        $parts.Add($direction + [string] $parameter.name + ': ' +
            [string] $parameter.type + $optional + $default)
    }
    return $parts -join ', '
}

function Add-Header {
    param($Lines, [string] $Id, [string] $Title)
    $Lines.Add("# $Id — $Title")
    $Lines.Add('')
    $Lines.Add('**Content type:** Project requirement')
    $Lines.Add('')
    $Lines.Add('**Status:** Proposed')
    $Lines.Add('')
    $Lines.Add('**Source:** Installed OLE Automation standard type library (`stdole2.tlb`)')
    $Lines.Add('')
}

function Add-MemberTable {
    param($Lines, [string] $Heading, $Members)
    $Lines.Add("## $Heading")
    $Lines.Add('')
    $Lines.Add('| Member | Kind | Parameters | Returns | Member ID |')
    $Lines.Add('| --- | --- | --- | --- | ---: |')
    foreach ($member in @($Members)) {
        $Lines.Add('| `' + (Escape-Markdown ([string] $member.name)) + '` | ' +
            (Escape-Markdown ([string] $member.kind)) + ' | `' +
            (Escape-Markdown (Get-ParameterText $member.parameters)) + '` | `' +
            (Escape-Markdown ([string] $member.returnType)) + '` | ' +
            [string] $member.memberId + ' |')
    }
    $Lines.Add('')
}

function Add-Footer {
    param($Lines, [string] $Evidence, [string] $Verification)
    $Lines.Add('## Rationale')
    $Lines.Add('')
    $Lines.Add('VB6 forms and intrinsic controls exchange standard OLE font and picture')
    $Lines.Add('objects. Preserving these contracts is required for source, Automation,')
    $Lines.Add('persistence, and binary interoperability.')
    $Lines.Add('')
    $Lines.Add('## Verification')
    $Lines.Add('')
    $Lines.Add('**Method:** Inspection and test')
    $Lines.Add('')
    $Lines.Add("**References:** ``$Evidence``; test case TBD")
    $Lines.Add('')
    $Lines.Add($Verification)
    $Lines.Add('')
    $Lines.Add('## Relationships')
    $Lines.Add('')
    $Lines.Add('- **Derived from:** MP-0001 compatibility discovery objective')
    $Lines.Add('- **Depends on:** COM and OLE Automation compatibility architecture')
    $Lines.Add('- **Conflicts with:** None known')
    $Lines.Add('')
    $Lines.Add('## Tailoring')
    $Lines.Add('')
    $Lines.Add('A listed interface or member may be deferred only through an approved')
    $Lines.Add('compatibility-scope change identifying the affected VB6 features.')
    $Lines.Add('')
    $Lines.Add('## Implementation Record')
    $Lines.Add('')
    $Lines.Add('Not implemented.')
}

$inventory = Get-Content -LiteralPath $InventoryPath -Raw | ConvertFrom-Json
$resolvedOutput = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($resolvedOutput) | Out-Null
$evidence = 'evidence/reference/stdole-2.0-type-library.json'

$contracts = @(
    [ordered]@{ id = 'REQ-0095'; file = 'req-0095-std-font-api-contract.md'; class = 'StdFont'; interfaces = @('IFont', 'Font'); title = 'stdole StdFont API contract' },
    [ordered]@{ id = 'REQ-0096'; file = 'req-0096-std-picture-api-contract.md'; class = 'StdPicture'; interfaces = @('IPicture', 'Picture'); title = 'stdole StdPicture API contract' }
)

foreach ($contract in $contracts) {
    $class = $inventory.classes | Where-Object name -eq $contract.class | Select-Object -First 1
    if ($null -eq $class) { throw "Missing stdole class: $($contract.class)" }
    $lines = [System.Collections.Generic.List[string]]::new()
    Add-Header $lines $contract.id $contract.title
    $lines.Add('## Scope')
    $lines.Add('')
    $lines.Add("This requirement applies to the WFC implementation of ``$($contract.class)`` on supported Windows targets.")
    $lines.Add('')
    $lines.Add('## Requirement')
    $lines.Add('')
    $lines.Add("WFC shall provide the ``$($contract.class)`` coclass with the reference CLSID,")
    $lines.Add('ProgID, Automation interface, native COM interface, event interface where')
    $lines.Add('present, member ordering, member identifiers, and signatures listed below.')
    $lines.Add('')
    $lines.Add('## Reference Identity')
    $lines.Add('')
    $lines.Add('| Item | Observed value |')
    $lines.Add('| --- | --- |')
    $lines.Add("| Type library | ``stdole`` 2.0, ``$($inventory.library.guid)`` |")
    $lines.Add("| CLSID | ``$($class.guid)`` |")
    $lines.Add("| ProgID | ``$($class.progId)`` |")
    $lines.Add('')
    foreach ($interfaceName in $contract.interfaces) {
        $interface = $inventory.interfaces | Where-Object name -eq $interfaceName | Select-Object -First 1
        if ($null -eq $interface) { throw "Missing stdole interface: $interfaceName" }
        Add-MemberTable $lines ("Interface ``$interfaceName`` (``$($interface.guid)``)") $interface.members
    }
    if ($null -ne $class.defaultEventInterface) {
        Add-MemberTable $lines ("Event interface ``$($class.defaultEventInterface.name)`` (``$($class.defaultEventInterface.guid)``)") $class.defaultEventInterface.members
    }
    Add-Footer $lines $evidence 'Compare the emitted type information and vtable layout with the retained inventory, then exercise object creation, property mutation, notification, persistence, and rendering in reference and WFC tests.'
    [IO.File]::WriteAllLines((Join-Path $resolvedOutput $contract.file), $lines,
        [Text.UTF8Encoding]::new($false))
}

$declaration = $inventory.declarations | Where-Object name -eq 'StdFunctions' | Select-Object -First 1
$lines = [System.Collections.Generic.List[string]]::new()
Add-Header $lines 'REQ-0097' 'stdole picture persistence functions'
$lines.Add('## Scope')
$lines.Add('')
$lines.Add('This requirement applies to VB-visible standard picture loading and saving.')
$lines.Add('')
$lines.Add('## Requirement')
$lines.Add('')
$lines.Add('WFC shall expose `LoadPicture` and `SavePicture` with the names, signatures,')
$lines.Add('optional arguments, defaults, return types, and member identifiers below.')
$lines.Add('The implementation shall preserve reference file-format selection, requested')
$lines.Add('size conversion, palette-mode flags, error reporting, and picture identity.')
$lines.Add('')
Add-MemberTable $lines 'Function Inventory' $declaration.members
Add-Footer $lines $evidence 'Compare type information and use fixtures covering empty input, missing and malformed files, each supported picture format, requested dimensions, flags, save/reload equivalence, and failure codes.'
[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'req-0097-picture-persistence-functions.md'), $lines,
    [Text.UTF8Encoding]::new($false))

$lines = [System.Collections.Generic.List[string]]::new()
Add-Header $lines 'REQ-0098' 'stdole constants contract'
$lines.Add('## Scope')
$lines.Add('')
$lines.Add('This requirement applies to all constants exposed by the standard OLE library.')
$lines.Add('')
$lines.Add('## Requirement')
$lines.Add('')
$lines.Add('WFC shall expose the following enumeration names, member names, integral')
$lines.Add('values, and source-visible types.')
$lines.Add('')
foreach ($constant in @($inventory.constants)) {
    $lines.Add("### ``$($constant.name)`` (``$($constant.guid)``)")
    $lines.Add('')
    $lines.Add('| Name | Value | Type | Member ID |')
    $lines.Add('| --- | ---: | --- | ---: |')
    foreach ($member in @($constant.members)) {
        $lines.Add("| ``$($member.name)`` | $($member.value) | ``$($member.type)`` | $($member.memberId) |")
    }
    $lines.Add('')
}
Add-Footer $lines $evidence 'Compile representative VB6 source against the WFC declarations and compare each integral value with the retained inventory.'
[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'req-0098-constants-api-contract.md'), $lines,
    [Text.UTF8Encoding]::new($false))

$index = @(
    '# Standard OLE Requirements',
    '',
    '**Content type:** Project requirement index',
    '',
    '**Status:** Proposed',
    '',
    '**Reference baseline:** Installed `stdole2.tlb`, standard OLE type library 2.0',
    '',
    '| Requirement | Contract |',
    '| --- | --- |',
    '| `REQ-0095` | [StdFont](req-0095-std-font-api-contract.md) |',
    '| `REQ-0096` | [StdPicture](req-0096-std-picture-api-contract.md) |',
    '| `REQ-0097` | [Picture persistence functions](req-0097-picture-persistence-functions.md) |',
    '| `REQ-0098` | [Constants](req-0098-constants-api-contract.md) |',
    '',
    'These requirements include both the VB-visible Automation surfaces and the',
    'native `IFont` and `IPicture` interfaces used by COM containers and controls.'
)
[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'README.md'), $index,
    [Text.UTF8Encoding]::new($false))
