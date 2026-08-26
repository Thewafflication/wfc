[CmdletBinding()]
param(
    [Parameter()]
    [string] $InventoryPath = '.\evidence\reference\vba-6.0-type-library.json',

    [Parameter()]
    [string] $OutputDirectory = '.\requirements\vba'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$items = @(
    @{ id = 69; kind = 'class'; name = 'ErrObject' },
    @{ id = 70; kind = 'class'; name = 'Collection' },
    @{ id = 71; kind = 'module'; name = 'Strings' },
    @{ id = 72; kind = 'module'; name = 'Conversion' },
    @{ id = 73; kind = 'module'; name = 'FileSystem' },
    @{ id = 74; kind = 'module'; name = 'DateTime' },
    @{ id = 75; kind = 'module'; name = 'Information' },
    @{ id = 76; kind = 'module'; name = 'Interaction' },
    @{ id = 77; kind = 'module'; name = 'Math' },
    @{ id = 78; kind = 'module'; name = 'Financial' },
    @{ id = 79; kind = 'module'; name = '_HiddenModule' },
    @{ id = 80; kind = 'enum'; name = 'VbVarType' },
    @{ id = 81; kind = 'enum'; name = 'VbMsgBoxStyle' },
    @{ id = 82; kind = 'enum'; name = 'VbMsgBoxResult' },
    @{ id = 83; kind = 'enum'; name = 'VbFileAttribute' },
    @{ id = 84; kind = 'enum'; name = 'VbStrConv' },
    @{ id = 85; kind = 'enum'; name = 'VbDayOfWeek' },
    @{ id = 86; kind = 'enum'; name = 'VbFirstWeekOfYear' },
    @{ id = 87; kind = 'enum'; name = 'VbIMEStatus' },
    @{ id = 88; kind = 'enum'; name = 'VbAppWinStyle' },
    @{ id = 89; kind = 'enum'; name = 'VbCompareMethod' },
    @{ id = 90; kind = 'enum'; name = 'VbCalendar' },
    @{ id = 91; kind = 'enum'; name = 'VbDateTimeFormat' },
    @{ id = 92; kind = 'enum'; name = 'VbTriState' },
    @{ id = 93; kind = 'enum'; name = 'VbCallType' },
    @{ id = 94; kind = 'enum'; name = 'Constants' }
)

function Escape-Markdown {
    param([AllowEmptyString()][string] $Text)
    if ($null -eq $Text) { return '' }
    return $Text.Replace('|', '\|').Replace("`r", '').Replace("`n", ' ')
}

function Get-Slug {
    param([string] $Name)
    $value = $Name.TrimStart('_')
    return ([regex]::Replace($value, '([a-z0-9])([A-Z])', '$1-$2')).ToLowerInvariant()
}

function Get-FlagNames {
    param([int] $Flags)
    $names = [System.Collections.Generic.List[string]]::new()
    if (($Flags -band 4) -ne 0) { $names.Add('Bindable') }
    if (($Flags -band 8) -ne 0) { $names.Add('RequestEdit') }
    if (($Flags -band 64) -ne 0) { $names.Add('Hidden') }
    if (($Flags -band 1024) -ne 0) { $names.Add('NonBrowsable') }
    if ($names.Count -eq 0) { return '—' }
    return $names -join ', '
}

function Get-ValueText {
    param($Value)
    if ($null -eq $Value) { return 'Empty' }
    if ($Value -is [bool]) { return $(if ($Value) { 'True' } else { 'False' }) }
    if ($Value -is [string]) {
        $escaped = $Value.Replace('\', '\\').Replace('"', '""')
        $escaped = $escaped.Replace("`0", '\0').Replace("`r", '\r').Replace("`n", '\n')
        $escaped = $escaped.Replace("`t", '\t').Replace("`b", '\b').Replace("`f", '\f')
        $escaped = $escaped.Replace([string][char]11, '\v')
        return '"' + $escaped + '"'
    }
    return [Convert]::ToString($Value, [Globalization.CultureInfo]::InvariantCulture)
}

function Get-ParameterText {
    param($Parameters)
    $parts = [System.Collections.Generic.List[string]]::new()
    foreach ($parameter in $Parameters) {
        $prefix = if (([int] $parameter.flags -band 2) -ne 0) { 'ByRef ' } else { '' }
        $suffix = ''
        if ([bool] $parameter.hasDefault) {
            $suffix = ' = ' + (Get-ValueText $parameter.defaultValue)
        }
        elseif ([bool] $parameter.optional) {
            $suffix = ' optional'
        }
        $parts.Add($prefix + [string] $parameter.name + ': ' +
            [string] $parameter.type + $suffix)
    }
    return $parts -join ', '
}

function Add-RequirementPreamble {
    param(
        [System.Collections.Generic.List[string]] $Lines,
        [string] $Id,
        [string] $Title,
        [string] $Scope,
        [string] $Requirement
    )
    $Lines.Add("# $Id — $Title")
    $Lines.Add('')
    $Lines.Add('**Content type:** Project requirement')
    $Lines.Add('')
    $Lines.Add('**Status:** Proposed')
    $Lines.Add('')
    $Lines.Add('**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)')
    $Lines.Add('')
    $Lines.Add('## Scope')
    $Lines.Add('')
    $Lines.Add($Scope)
    $Lines.Add('')
    $Lines.Add('## Requirement')
    $Lines.Add('')
    $Lines.Add($Requirement)
}

function Add-RequirementTail {
    param(
        [System.Collections.Generic.List[string]] $Lines,
        [string] $DependsOn
    )
    $Lines.Add('')
    $Lines.Add('## Rationale')
    $Lines.Add('')
    $Lines.Add('This contract is part of the source-visible VBA standard library required')
    $Lines.Add('to compile and execute compatible VB and VBA code.')
    $Lines.Add('')
    $Lines.Add('## Verification')
    $Lines.Add('')
    $Lines.Add('**Method:** Inspection')
    $Lines.Add('')
    $Lines.Add('**References:** `evidence/reference/vba-6.0-type-library.json`; test case TBD')
    $Lines.Add('')
    $Lines.Add('Inspect the WFC VBA type information and compare every listed name, value,')
    $Lines.Add('member identifier, access mode, and signature. Function behavior requires')
    $Lines.Add('separate executable reference and conformance tests.')
    $Lines.Add('')
    $Lines.Add('## Relationships')
    $Lines.Add('')
    $Lines.Add('- **Derived from:** MP-0001 compatibility discovery objective')
    $Lines.Add("- **Depends on:** $DependsOn")
    $Lines.Add('- **Conflicts with:** None known')
    $Lines.Add('')
    $Lines.Add('## Tailoring')
    $Lines.Add('')
    $Lines.Add('A member or value may be deferred only through an approved compatibility-scope')
    $Lines.Add('change that identifies the resulting source or run-time limitation.')
    $Lines.Add('')
    $Lines.Add('## Implementation Record')
    $Lines.Add('')
    $Lines.Add('Not implemented.')
}

$inventory = Get-Content -LiteralPath $InventoryPath -Raw | ConvertFrom-Json
$resolvedOutput = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($resolvedOutput) | Out-Null
$indexRows = [System.Collections.Generic.List[string]]::new()

foreach ($item in $items) {
    $id = 'REQ-{0:D4}' -f $item.id
    $slug = Get-Slug $item.name
    $fileName = ('req-{0:D4}-{1}-api-contract.md' -f $item.id, $slug)
    $lines = [System.Collections.Generic.List[string]]::new()

    if ($item.kind -eq 'class') {
        $model = $inventory.classes | Where-Object name -eq $item.name | Select-Object -First 1
        if ($null -eq $model) { throw "Missing VBA class $($item.name)" }
        $properties = @($model.defaultInterface.members |
            Where-Object { $_.kind -like 'Property*' } | Group-Object name)
        $methods = @($model.defaultInterface.members | Where-Object kind -eq 'Method')

        Add-RequirementPreamble $lines $id "VBA $($item.name) class API contract" `
            "This requirement applies to the VBA ``$($item.name)`` run-time class." `
            "The WFC VBA runtime shall expose the ``$($item.name)`` properties and methods listed below with the specified names, DISPIDs, access modes, and signatures."
        $lines.Add('')
        $lines.Add('## Reference Identity')
        $lines.Add('')
        $lines.Add('| Item | Observed value |')
        $lines.Add('| --- | --- |')
        $lines.Add("| Reference CLSID | ``$($model.guid)`` |")
        $lines.Add("| Default interface | ``$($model.defaultInterface.name)``, ``$($model.defaultInterface.guid)`` |")
        $lines.Add('')
        $lines.Add('## Property Inventory')
        $lines.Add('')
        $lines.Add('| Property | Access | Type | DISPID | Flags |')
        $lines.Add('| --- | --- | --- | ---: | --- |')
        foreach ($property in $properties) {
            $entries = @($property.Group)
            $getter = $entries | Where-Object kind -eq 'PropertyGet' | Select-Object -First 1
            $representative = if ($null -ne $getter) { $getter } else { $entries[0] }
            $access = ($entries.kind | ForEach-Object { $_.Substring(8) } | Select-Object -Unique) -join '/'
            $parameterText = Get-ParameterText $representative.parameters
            $displayName = [string] $property.Name
            if (-not [string]::IsNullOrEmpty($parameterText)) { $displayName += '(' + $parameterText + ')' }
            $lines.Add('| `' + (Escape-Markdown $displayName) + '` | ' + $access +
                ' | `' + (Escape-Markdown ([string] $representative.returnType)) + '` | ' +
                [string] $representative.memberId + ' | ' +
                (Get-FlagNames ([int] $representative.flags)) + ' |')
        }
        $lines.Add('')
        $lines.Add('## Method Inventory')
        $lines.Add('')
        $lines.Add('| Method | Parameters | Returns | DISPID | Flags |')
        $lines.Add('| --- | --- | --- | ---: | --- |')
        foreach ($method in $methods) {
            $lines.Add('| `' + (Escape-Markdown ([string] $method.name)) + '` | `' +
                (Escape-Markdown (Get-ParameterText $method.parameters)) + '` | `' +
                (Escape-Markdown ([string] $method.returnType)) + '` | ' +
                [string] $method.memberId + ' | ' + (Get-FlagNames ([int] $method.flags)) + ' |')
        }
        Add-RequirementTail $lines 'VBA object and error model architecture'
        $indexRows.Add("| ``$id`` | Class | [$($item.name)]($fileName) | $($properties.Count + $methods.Count) |")
    }
    elseif ($item.kind -eq 'module') {
        $model = $inventory.declarations | Where-Object name -eq $item.name | Select-Object -First 1
        if ($null -eq $model) { throw "Missing VBA module $($item.name)" }

        Add-RequirementPreamble $lines $id "VBA $($item.name) module API contract" `
            "This requirement applies to the VBA ``$($item.name)`` standard-library module." `
            "The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values."
        $lines.Add('')
        $lines.Add('## Member Inventory')
        $lines.Add('')
        $lines.Add('| Member | Kind | Parameters | Returns | DISPID | Flags |')
        $lines.Add('| --- | --- | --- | --- | ---: | --- |')
        foreach ($member in $model.members) {
            $lines.Add('| `' + (Escape-Markdown ([string] $member.name)) + '` | ' +
                [string] $member.kind + ' | `' +
                (Escape-Markdown (Get-ParameterText $member.parameters)) + '` | `' +
                (Escape-Markdown ([string] $member.returnType)) + '` | ' +
                [string] $member.memberId + ' | ' + (Get-FlagNames ([int] $member.flags)) + ' |')
        }
        Add-RequirementTail $lines 'VBA value, string, date, file, and host interaction architecture'
        $indexRows.Add("| ``$id`` | Module | [$($item.name)]($fileName) | $(@($model.members).Count) |")
    }
    else {
        $model = $inventory.constants | Where-Object name -eq $item.name | Select-Object -First 1
        if ($null -eq $model) { throw "Missing VBA enumeration $($item.name)" }

        Add-RequirementPreamble $lines $id "VBA $($item.name) values" `
            "This requirement applies wherever VBA exposes the ``$($item.name)`` constants." `
            "The WFC VBA runtime shall expose each listed constant with the specified name, value, and member identifier."
        $lines.Add('')
        $lines.Add('## Value Inventory')
        $lines.Add('')
        $lines.Add('| Constant | Value | Member ID |')
        $lines.Add('| --- | ---: | ---: |')
        foreach ($member in $model.members) {
            $valueText = Escape-Markdown (Get-ValueText $member.value)
            $lines.Add("| ``$($member.name)`` | ``$valueText`` | $($member.memberId) |")
        }
        Add-RequirementTail $lines 'VBA constant-expression and enumeration support'
        $indexRows.Add("| ``$id`` | Enumeration | [$($item.name)]($fileName) | $(@($model.members).Count) |")
    }

    [IO.File]::WriteAllLines((Join-Path $resolvedOutput $fileName), $lines,
        [Text.UTF8Encoding]::new($false))
}

$index = [System.Collections.Generic.List[string]]::new()
$index.Add('# Visual Basic for Applications 6.0 Requirements')
$index.Add('')
$index.Add('**Content type:** Project requirement index')
$index.Add('')
$index.Add('**Status:** Proposed')
$index.Add('')
$index.Add('**Reference baseline:** Installed `VBA6.DLL` 6.0.8169, VBA type library 6.0')
$index.Add('')
$index.Add('This set captures the two VBA runtime classes, nine standard-library modules,')
$index.Add('and fifteen enumeration or constant groups exposed by the installed library.')
$index.Add('The `_HiddenModule` contract is retained because compatibility may depend on')
$index.Add('members that are not displayed by ordinary object browsers.')
$index.Add('')
$index.Add('Names such as `_B_str_Chr` and `_B_var_Chr` are type-library entry-point')
$index.Add('names for typed VBA forms such as `Chr$` and `Chr`. Source spelling and')
$index.Add('overload resolution belong to the future VBA language requirements; this set')
$index.Add('preserves the inspected binary contract exactly.')
$index.Add('')
$index.Add('| Requirement | Kind | Contract | Members |')
$index.Add('| --- | --- | --- | ---: |')
foreach ($row in $indexRows) { $index.Add($row) }
$index.Add('')
$index.Add('## Evidence Boundary')
$index.Add('')
$index.Add('The type library establishes the callable surface and constant values. Detailed')
$index.Add('string, conversion, date, file-system, financial, interaction, error, locale,')
$index.Add('and host-dependent behavior requires requirements derived from `VBA.HLP` and')
$index.Add('confirmed by executable reference probes.')

[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'README.md'), $index,
    [Text.UTF8Encoding]::new($false))
