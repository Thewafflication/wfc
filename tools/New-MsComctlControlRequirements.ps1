[CmdletBinding()]
param(
    [Parameter()]
    [string] $InventoryPath = '.\evidence\reference\mscomctl-2.0-type-library.json',

    [Parameter()]
    [string] $OutputDirectory = '.\requirements\controls'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$definitions = [ordered]@{
    TabStrip = @{
        id = 'REQ-0001'; file = 'req-0001-tabstrip-api-contract.md'
        capabilities = @(
            'Tab collection creation, removal, lookup, selection, and multi-selection'
            'Tab layout, sizing, placement, separators, styles, images, and tooltips'
            'Keyboard, pointer, selection, and OLE drag-and-drop event behavior'
        )
    }
    Toolbar = @{
        id = 'REQ-0002'; file = 'req-0002-toolbar-api-contract.md'
        capabilities = @(
            'Button and button-menu collection management, layout, images, and styles'
            'Customization plus toolbar state save and restore'
            'Button, menu, dropdown, pointer, and OLE drag-and-drop event behavior'
        )
    }
    StatusBar = @{
        id = 'REQ-0003'; file = 'req-0003-statusbar-api-contract.md'
        capabilities = @(
            'Simple-text and panel-based display modes'
            'Panel collection content, sizing, style, images, and tooltip behavior'
            'Panel, pointer, and OLE drag-and-drop event behavior'
        )
    }
    ProgressBar = @{
        id = 'REQ-0004'; file = 'req-0004-progressbar-api-contract.md'
        capabilities = @(
            'Minimum, maximum, and current-value range semantics including invalid values'
            'Horizontal and vertical layout, scrolling mode, appearance, and borders'
            'Pointer and OLE drag-and-drop event behavior'
        )
    }
    TreeView = @{
        id = 'REQ-0005'; file = 'req-0005-treeview-api-contract.md'
        capabilities = @(
            'Hierarchical node collection creation, removal, lookup, and traversal'
            'Selection, expansion, checkboxes, sorting, label editing, images, and hit testing'
            'Node, keyboard, pointer, label-edit, and OLE drag-and-drop event ordering'
        )
    }
    ListView = @{
        id = 'REQ-0006'; file = 'req-0006-listview-api-contract.md'
        capabilities = @(
            'List item, subitem, and column-header collection management'
            'Icon, small-icon, list, and report views with sorting, selection, checks, and label editing'
            'Search, hit testing, scrolling, images, rendering options, and event ordering'
        )
    }
    ImageList = @{
        id = 'REQ-0007'; file = 'req-0007-imagelist-api-contract.md'
        capabilities = @(
            'Image collection creation, removal, lookup, extraction, and replacement'
            'Image dimensions, mask color, background color, overlay images, and native handle behavior'
            'Picture format, transparency, scaling, indexing, and error semantics'
        )
    }
    Slider = @{
        id = 'REQ-0008'; file = 'req-0008-slider-api-contract.md'
        capabilities = @(
            'Minimum, maximum, value, small-change, and large-change range semantics'
            'Ticks, selection range, orientation, text, appearance, and programmatic clicks'
            'Keyboard, pointer, scroll, change, and OLE drag-and-drop event ordering'
        )
    }
    ImageCombo = @{
        id = 'REQ-0009'; file = 'req-0009-imagecombo-api-contract.md'
        capabilities = @(
            'Combo-item collection creation, removal, lookup, images, and indentation'
            'Text editing, selection, locking, dropdown, scrolling, colors, and fonts'
            'Keyboard, selection, dropdown, and OLE drag-and-drop event behavior'
        )
    }
}

function Escape-Markdown {
    param([AllowEmptyString()][string] $Text)
    if ($null -eq $Text) { return '' }
    return $Text.Replace('|', '\|').Replace("`r", '').Replace("`n", ' ')
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

function Get-ParameterText {
    param($Parameters)
    $parts = [System.Collections.Generic.List[string]]::new()
    foreach ($parameter in $Parameters) {
        $prefix = ''
        if (([int] $parameter.flags -band 2) -ne 0) { $prefix = 'ByRef ' }
        $suffix = ''
        if ([bool] $parameter.optional) { $suffix = ' optional' }
        $parts.Add($prefix + [string] $parameter.name + ': ' + [string] $parameter.type + $suffix)
    }
    return $parts -join ', '
}

$inventory = Get-Content -LiteralPath $InventoryPath -Raw | ConvertFrom-Json
$resolvedOutput = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($resolvedOutput) | Out-Null

$indexRows = [System.Collections.Generic.List[string]]::new()

foreach ($control in $inventory.controls) {
    $definition = $definitions[[string] $control.name]
    if ($null -eq $definition) {
        throw 'No requirement definition exists for control ' + [string] $control.name
    }

    $properties = $control.defaultInterface.members |
        Where-Object { $_.kind -like 'Property*' } |
        Group-Object name
    $methods = $control.defaultInterface.members | Where-Object { $_.kind -eq 'Method' }
    $events = @($control.defaultEventInterface.members)

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add('# ' + $definition.id + ' — ' + $control.name + ' Automation API contract')
    $lines.Add('')
    $lines.Add('**Content type:** Project requirement  ')
    $lines.Add('**Status:** Proposed  ')
    $lines.Add('**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline')
    $lines.Add('')
    $lines.Add('## Scope')
    $lines.Add('')
    $lines.Add('This requirement applies to the WFC compatibility implementation of the')
    $lines.Add('`' + $control.name + '` ActiveX control on Windows x86, x64, and ARM64. The')
    $lines.Add('inventory below describes the reference x86 Automation contract. Reference')
    $lines.Add('GUIDs identify the observed component but do not require WFC to reuse them;')
    $lines.Add('binary identity and registration coexistence require a separate ADR.')
    $lines.Add('')
    $lines.Add('## Requirement')
    $lines.Add('')
    $lines.Add('The WFC `' + $control.name + '` compatibility component shall expose the')
    $lines.Add('properties, methods, and events listed in this document with the specified')
    $lines.Add('Automation names, member identifiers, access modes, and parameter and return')
    $lines.Add('types. Hidden and non-browsable listed members are part of this contract.')
    $lines.Add('')
    $lines.Add('## Reference Identity')
    $lines.Add('')
    $lines.Add('| Item | Observed value |')
    $lines.Add('| --- | --- |')
    $lines.Add('| Type library | `MSComctlLib` 2.0, `' + $inventory.library.guid + '` |')
    $lines.Add('| ProgID | `' + $control.progId + '` |')
    $lines.Add('| Reference CLSID | `' + $control.guid + '` |')
    $lines.Add('| Default interface | `' + $control.defaultInterface.name + '`, `' + $control.defaultInterface.guid + '` |')
    if ($events.Count -gt 0) {
        $lines.Add('| Default event interface | `' + $control.defaultEventInterface.name + '`, `' + $control.defaultEventInterface.guid + '` |')
    }
    else {
        $lines.Add('| Default event interface | None exposed |')
    }
    $lines.Add('')
    $lines.Add('## Property Inventory')
    $lines.Add('')
    $lines.Add('| Property | Access | Type | DISPID | Flags |')
    $lines.Add('| --- | --- | --- | ---: | --- |')
    foreach ($property in $properties) {
        $entries = @($property.Group)
        $getter = $entries | Where-Object { $_.kind -eq 'PropertyGet' } | Select-Object -First 1
        $representative = if ($null -ne $getter) { $getter } else { $entries[0] }
        $access = ($entries.kind | ForEach-Object { $_.Substring(8) } | Select-Object -Unique) -join '/'
        $lines.Add('| `' + (Escape-Markdown $property.Name) + '` | ' + $access + ' | `' +
            (Escape-Markdown ([string] $representative.returnType)) + '` | ' +
            [string] $representative.memberId + ' | ' + (Get-FlagNames ([int] $representative.flags)) + ' |')
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
    $lines.Add('')
    $lines.Add('## Event Inventory')
    $lines.Add('')
    if ($events.Count -eq 0) {
        $lines.Add('The reference control exposes no members on its default event interface.')
    }
    else {
        $lines.Add('| Event | Parameters | DISPID |')
        $lines.Add('| --- | --- | ---: |')
        foreach ($event in $events) {
            $lines.Add('| `' + (Escape-Markdown ([string] $event.name)) + '` | `' +
                (Escape-Markdown (Get-ParameterText $event.parameters)) + '` | ' +
                [string] $event.memberId + ' |')
        }
    }
    $lines.Add('')
    $lines.Add('## Behavioral Characterization Needed')
    $lines.Add('')
    $lines.Add('The type library establishes structure, not runtime semantics. Follow-on')
    $lines.Add('behavioral requirements and VB6 reference probes shall cover:')
    $lines.Add('')
    foreach ($capability in $definition.capabilities) {
        $lines.Add('- ' + $capability + '.')
    }
    $lines.Add('- Default values, boundary values, invalid inputs, error numbers, persistence,')
    $lines.Add('  rendering, focus, keyboard navigation, and behavior at common DPI settings.')
    $lines.Add('')
    $lines.Add('## Rationale')
    $lines.Add('')
    $lines.Add('Matching the published Automation surface is necessary for source-level VB6')
    $lines.Add('compatibility and provides a stable boundary for later behavioral probes.')
    $lines.Add('')
    $lines.Add('## Verification')
    $lines.Add('')
    $lines.Add('**Method:** Inspection')
    $lines.Add('')
    $lines.Add('**References:** `evidence/reference/mscomctl-2.0-type-library.json`; test case TBD')
    $lines.Add('')
    $lines.Add('Inspect the WFC type library and compare every listed member name, DISPID,')
    $lines.Add('access mode, signature, and event against this requirement. Runtime semantics')
    $lines.Add('are verified by separate behavioral requirements and tests.')
    $lines.Add('')
    $lines.Add('## Relationships')
    $lines.Add('')
    $lines.Add('- **Derived from:** MP-0001 compatibility discovery objective')
    $lines.Add('- **Depends on:** WCRT integration ADR and COM identity/coexistence ADR')
    $lines.Add('- **Conflicts with:** None known')
    $lines.Add('')
    $lines.Add('## Tailoring')
    $lines.Add('')
    $lines.Add('A member may be deferred only through an approved compatibility-scope change')
    $lines.Add('that identifies the resulting limitation.')
    $lines.Add('')
    $lines.Add('## Implementation Record')
    $lines.Add('')
    $lines.Add('Not implemented.')

    $target = Join-Path $resolvedOutput $definition.file
    [IO.File]::WriteAllLines($target, $lines, [Text.UTF8Encoding]::new($false))
    $indexRows.Add('| `' + $definition.id + '` | [' + $control.name + '](' + $definition.file +
        ') | `' + $control.progId + '` | ' + $properties.Count + ' | ' + $methods.Count +
        ' | ' + $events.Count + ' | Proposed |')
}

$index = [System.Collections.Generic.List[string]]::new()
$index.Add('# MSComctlLib Control Requirements')
$index.Add('')
$index.Add('**Content type:** Project requirement index  ')
$index.Add('**Status:** Proposed  ')
$index.Add('**Reference baseline:** `MSCOMCTL.OCX` 6.00.8177, MSComctlLib 2.0')
$index.Add('')
$index.Add('These requirements capture the Automation API surface of every control class')
$index.Add('marked as a control in the installed type library. Supporting collections and')
$index.Add('value-object interfaces are referenced by these contracts and will receive')
$index.Add('separate requirements as behavioral probes are designed.')
$index.Add('')
$index.Add('| Requirement | Control | Reference ProgID | Properties | Methods | Events | Status |')
$index.Add('| --- | --- | --- | ---: | ---: | ---: | --- |')
foreach ($row in $indexRows) { $index.Add($row) }
$index.Add('')
$index.Add('## Evidence Boundary')
$index.Add('')
$index.Add('The inventories are generated from `evidence/reference/mscomctl-2.0-type-library.json`.')
$index.Add('They establish names, DISPIDs, access modes, and Automation signatures. They do')
$index.Add('not establish default values, state transitions, rendering, event order, error')
$index.Add('semantics, persistence, or accessibility behavior; those require executable VB6')
$index.Add('reference probes and separately verifiable requirements.')
$index.Add('')
$index.Add('## Behavioral Requirements')
$index.Add('')
$index.Add('The [behavioral requirement set](behavior/README.md) defines the first 27')
$index.Add('observable control behaviors derived from the installed Microsoft help. These')
$index.Add('requirements cover all nine control classes and are allocated to planned')
$index.Add('matching-number VB6 reference and WFC conformance tests.')

[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'README.md'), $index, [Text.UTF8Encoding]::new($false))
