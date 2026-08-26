[CmdletBinding()]
param(
    [Parameter()]
    [string] $InventoryPath = '.\evidence\reference\vb-6.0-type-library.json',

    [Parameter()]
    [string] $OutputDirectory = '.\requirements\vb\classes'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$classOrder = @(
    'PictureBox', 'Label', 'TextBox', 'Frame', 'CommandButton', 'CheckBox',
    'OptionButton', 'ComboBox', 'ListBox', 'HScrollBar', 'VScrollBar', 'Timer',
    'Printer', 'Form', 'Screen', 'Clipboard', 'DriveListBox', 'DirListBox',
    'FileListBox', 'Menu', 'MDIForm', 'App', 'Shape', 'Line', 'Image', 'Data',
    'OLE', 'UserControl', 'PropertyPage', 'UserDocument', 'Global',
    'VBControlExtender'
)

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
        $prefix = if (([int] $parameter.flags -band 2) -ne 0) { 'ByRef ' } else { '' }
        $suffix = if ([bool] $parameter.optional) { ' optional' } else { '' }
        $parts.Add($prefix + [string] $parameter.name + ': ' +
            [string] $parameter.type + $suffix)
    }
    return $parts -join ', '
}

function Get-Slug {
    param([string] $Name)
    return ([regex]::Replace($Name, '([a-z0-9])([A-Z])', '$1-$2')).ToLowerInvariant()
}

$inventory = Get-Content -LiteralPath $InventoryPath -Raw | ConvertFrom-Json
$resolvedOutput = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($resolvedOutput) | Out-Null

$indexRows = [System.Collections.Generic.List[string]]::new()

for ($classIndex = 0; $classIndex -lt $classOrder.Count; $classIndex++) {
    $className = $classOrder[$classIndex]
    $class = $inventory.classes | Where-Object name -eq $className | Select-Object -First 1
    if ($null -eq $class) { throw "Missing expected VB class: $className" }

    $number = 37 + $classIndex
    $id = 'REQ-{0:D4}' -f $number
    $fileName = ('req-{0:D4}-{1}-api-contract.md' -f $number, (Get-Slug $className))
    $properties = @($class.defaultInterface.members |
        Where-Object { $_.kind -like 'Property*' } | Group-Object name)
    $methods = @($class.defaultInterface.members | Where-Object kind -eq 'Method')
    [array] $events = if ($null -eq $class.defaultEventInterface) {
        @()
    }
    else {
        @($class.defaultEventInterface.members)
    }

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("# $id — VB $className API contract")
    $lines.Add('')
    $lines.Add('**Content type:** Project requirement')
    $lines.Add('')
    $lines.Add('**Status:** Proposed')
    $lines.Add('')
    $lines.Add('**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)')
    $lines.Add('')
    $lines.Add('## Scope')
    $lines.Add('')
    $lines.Add("This requirement applies to the WFC compatibility implementation of the ``$className`` class on supported Windows targets.")
    $lines.Add('')
    $lines.Add('## Requirement')
    $lines.Add('')
    $lines.Add("The WFC ``$className`` class shall expose the properties, methods, and events")
    $lines.Add('listed below with the specified Automation names, member identifiers, access')
    $lines.Add('modes, and parameter and return types. Hidden and non-browsable listed members')
    $lines.Add('are part of the compatibility contract.')
    $lines.Add('')
    $lines.Add('## Reference Identity')
    $lines.Add('')
    $lines.Add('| Item | Observed value |')
    $lines.Add('| --- | --- |')
    $lines.Add("| Type library | ``VB`` 6.0, ``$($inventory.library.guid)`` |")
    $lines.Add("| Reference CLSID | ``$($class.guid)`` |")
    $lines.Add("| Default interface | ``$($class.defaultInterface.name)``, ``$($class.defaultInterface.guid)`` |")
    if (@($events).Count -gt 0) {
        $lines.Add("| Default event interface | ``$($class.defaultEventInterface.name)``, ``$($class.defaultEventInterface.guid)`` |")
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
        $getter = $entries | Where-Object kind -eq 'PropertyGet' | Select-Object -First 1
        $representative = if ($null -ne $getter) { $getter } else { $entries[0] }
        $access = ($entries.kind | ForEach-Object { $_.Substring(8) } | Select-Object -Unique) -join '/'
        $parameterText = Get-ParameterText $representative.parameters
        $displayName = [string] $property.Name
        if (-not [string]::IsNullOrEmpty($parameterText)) {
            $displayName += '(' + $parameterText + ')'
        }
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
            [string] $method.memberId + ' | ' +
            (Get-FlagNames ([int] $method.flags)) + ' |')
    }
    $lines.Add('')
    $lines.Add('## Event Inventory')
    $lines.Add('')
    if (@($events).Count -eq 0) {
        $lines.Add('The reference class exposes no members on its default event interface.')
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
    $lines.Add('## Rationale')
    $lines.Add('')
    $lines.Add('The VB object library defines the source-visible form, intrinsic-control, and')
    $lines.Add('run-time service classes required to load and execute VB6 form projects.')
    $lines.Add('')
    $lines.Add('## Verification')
    $lines.Add('')
    $lines.Add('**Method:** Inspection')
    $lines.Add('')
    $lines.Add("**References:** ``evidence/reference/vb-6.0-type-library.json``; test case TBD")
    $lines.Add('')
    $lines.Add('Inspect the WFC type information and compare every listed member name, DISPID,')
    $lines.Add('access mode, signature, and event. Behavioral compatibility requires separate')
    $lines.Add('requirements and executable VB6 reference tests.')
    $lines.Add('')
    $lines.Add('## Relationships')
    $lines.Add('')
    $lines.Add('- **Derived from:** MP-0001 compatibility discovery objective')
    $lines.Add('- **Depends on:** WCRT integration and form-runtime architecture decisions')
    $lines.Add('- **Conflicts with:** None known')
    $lines.Add('')
    $lines.Add('## Tailoring')
    $lines.Add('')
    $lines.Add('A member may be deferred only through an approved compatibility-scope change')
    $lines.Add('that identifies the resulting form or source-compatibility limitation.')
    $lines.Add('')
    $lines.Add('## Implementation Record')
    $lines.Add('')
    $lines.Add('Not implemented.')

    [IO.File]::WriteAllLines((Join-Path $resolvedOutput $fileName), $lines,
        [Text.UTF8Encoding]::new($false))
    $indexRows.Add("| ``$id`` | [$className]($fileName) | ``$($class.defaultInterface.name)`` | $($properties.Count) | $($methods.Count) | $(@($events).Count) |")
}

$index = [System.Collections.Generic.List[string]]::new()
$index.Add('# Visual Basic 6.0 Class Requirements')
$index.Add('')
$index.Add('**Content type:** Project requirement index')
$index.Add('')
$index.Add('**Status:** Proposed')
$index.Add('')
$index.Add('**Reference baseline:** Installed `VB6.OLB`, Visual Basic type library 6.0')
$index.Add('')
$index.Add('These requirements capture all 32 coclasses exposed by the VB run-time object')
$index.Add('library. `VB6EXT.OLB` is the separate VBIDE extensibility model and is outside')
$index.Add('this form-runtime requirement set.')
$index.Add('')
$index.Add('| Requirement | Class | Interface | Properties | Methods | Events |')
$index.Add('| --- | --- | --- | ---: | ---: | ---: |')
foreach ($row in $indexRows) { $index.Add($row) }
$index.Add('')
$index.Add('## Form Runtime Boundary')
$index.Add('')
$index.Add('The primary form classes are `Form` and `MDIForm`. `UserControl`,')
$index.Add('`PropertyPage`, and `UserDocument` define additional hosted visual surfaces;')
$index.Add('the intrinsic controls provide the standard children used by VB6 forms.')

[IO.File]::WriteAllLines((Join-Path $resolvedOutput 'README.md'), $index,
    [Text.UTF8Encoding]::new($false))
