<#
.SYNOPSIS
Generates the control probe: a Visual Basic 6 project with one form for each
control of MSComctlLib (TabStrip, Toolbar, StatusBar, ProgressBar, TreeView,
ListView, ImageList, Slider, ImageCombo) that records what the properties of
the control answer and what setting them does.

.DESCRIPTION
The properties come from the type library of MSCOMCTL.OCX
(evidence/reference/mscomctl-2.0-type-library.json). For every property of a
scalar type each probe reads the default, or sets a value and reads it back,
and returns "<TypeName>:<text>" or "err<number>". Main writes one line per
probe, "<control> | <property> | <action> = <result>", to control-probe.txt
next to the project. The value is put back after every set, so the probes do
not depend on each other.

.PARAMETER Output
The folder to write; the default is evidence\reference\control-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-ControlProbe.ps1
#>
[CmdletBinding()]
param(
    [ValidateSet('mscomctl', 'mscomctl2', 'comdlg32', 'mswinsck')]
    [string]$Library = 'mscomctl',
    [string]$Output = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path -Parent $PSScriptRoot
$libraries = @{
    mscomctl = @{
        Json = 'mscomctl-2.0-type-library.json'
        Namespace = 'MSComctlLib'
        Guid = '{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}'
        Ocx = 'MSCOMCTL.OCX'
        Folder = 'control-probe'
        Name = 'ControlProbe'
        Version = '2.0'
    }
    mscomctl2 = @{
        Json = 'mscomct2-2.0-type-library.json'
        Namespace = 'MSComCtl2'
        Guid = '{86CF1D34-0C5F-11D2-A9FC-0000F8754DA1}'
        Ocx = 'MSCOMCT2.OCX'
        Folder = 'control-probe-2'
        Name = 'ControlProbe2'
        Version = '2.0'
    }
    comdlg32 = @{
        Json = 'comdlg32-1.2-type-library.json'
        Namespace = 'MSComDlg'
        Guid = '{F9043C88-F6F2-101A-A3C9-08002B2F49FB}'
        Ocx = 'COMDLG32.OCX'
        Folder = 'control-probe-3'
        Name = 'ControlProbe3'
        Version = '1.2'
    }
    mswinsck = @{
        Json = 'mswinsck-1.0-type-library.json'
        Namespace = 'MSWinsockLib'
        Guid = '{248DD890-BB45-11CF-9ABC-0080C7E7B78D}'
        Ocx = 'MSWINSCK.OCX'
        Folder = 'control-probe-4'
        Name = 'ControlProbe4'
        Version = '1.0'
    }
}
$choice = $libraries[$Library]
if (-not $Output) {
    $Output = Join-Path $root "evidence\reference\$($choice.Folder)"
}
$typeLibrary = Get-Content -Raw -Encoding UTF8 (Join-Path $root `
    "evidence\reference\$($choice.Json)") | ConvertFrom-Json

# Properties that are objects, handles or depend on the window.
$skip = @('hWnd', 'Action', 'SocketHandle', 'LocalHostName', 'LocalIP',
    'hImageList', 'Font', 'Picture', 'MouseIcon', 'Controls',
    'ImageList', 'Icons', 'SmallIcons', 'DisabledImageList', 'HotImageList',
    'ColumnHeaderIcons', 'DropHighlight', 'SelectedItem', '_Value',
    'GetNumTicks', 'PanelProperties', 'ClientTop', 'ClientLeft',
    'ClientHeight', 'ClientWidth')

$booleans = @('True', 'False')
$whole = @('0', '1', '2', '3', '-1', '5', '99')
$wholeLong = $whole
$singles = @('0!', '1!', '0.5!', '-1!', '100!')
$colors = @('&H0&', '&HFF&', '&H80000005', '&HFFFFFF')
$strings = @('""', '"abc"')

function Get-ProbeValue([string]$Type) {
    if ($Type -eq 'Boolean') { return $booleans }
    if ($Type -eq 'Integer') { return $whole }
    if ($Type -eq 'Long' -or $Type.EndsWith('Constants')) {
        return $wholeLong
    }
    if ($Type -eq 'Single') { return $singles }
    if ($Type -eq 'ULong') { return $colors }
    if ($Type -eq 'String') { return $strings }
    return $null
}

$crlf = "`r`n"
$utf = New-Object System.Text.ASCIIEncoding
New-Item -ItemType Directory -Force -Path $Output | Out-Null

$objectLine = 'Object = "' + $choice.Guid + '#' + $choice.Version + '#0"; "' +
    $choice.Ocx + '"'
$count = 0
$forms = @()
# MSComctlLib and MSComCtl2 are exported with their controls; the other
# libraries with their classes, whose default interface holds the members.
$controls = if ($typeLibrary.PSObject.Properties.Name -contains 'controls') {
    $typeLibrary.controls
} else {
    $typeLibrary.classes
}
foreach ($control in $controls) {
    $name = $control.name
    $form = "frm$name"
    $forms += $form
    $members = $control.defaultInterface.members
    $properties = [ordered]@{}
    foreach ($member in $members) {
        if (-not $member.kind.StartsWith('Property')) { continue }
        if ($member.name.StartsWith('_')) { continue }
        if (@($member.parameters).Count -ne 0) { continue }
        if ($skip -contains $member.name) { continue }
        if (-not $properties.Contains($member.name)) {
            $properties[$member.name] = @{
                Type = $member.returnType; Kinds = @()
            }
        }
        $properties[$member.name].Kinds += $member.kind
    }
    $code = [System.Collections.Generic.List[string]]::new()
    $run = @('Public Sub Run()')
    foreach ($property in $properties.Keys) {
        $info = $properties[$property]
        $values = Get-ProbeValue $info.Type
        if ($null -eq $values) { continue }
        $readable = $info.Kinds -contains 'PropertyGet'
        $writable = ($info.Kinds -contains 'PropertyLet') -or
            ($info.Kinds -contains 'PropertySet')
        if ($readable) {
            $count++
            $id = 'P{0:D5}' -f $count
            $code.Add("Public Function $id() As String")
            $code.Add('    On Error GoTo h')
            $code.Add("    $id = Fmt(Ctl.$property)")
            $code.Add('    Exit Function')
            $code.Add('h:')
            $code.Add("    $id = `"err`" & Err.Number")
            $code.Add('End Function')
            $code.Add('')
            $run += "    If Want(`"$name | $property | get`") Then " +
                "Rec `"$name | $property | get`", $id()"
        }
        if ($writable -and $readable) {
            foreach ($value in $values) {
                $count++
                $id = 'P{0:D5}' -f $count
                $code.Add("Public Function $id() As String")
                $code.Add('    Dim d As Variant')
                $code.Add('    On Error Resume Next')
                $code.Add("    d = Ctl.$property")
                $code.Add('    On Error GoTo h')
                $code.Add("    Ctl.$property = $value")
                $code.Add("    $id = Fmt(Ctl.$property)")
                $code.Add('    On Error Resume Next')
                $code.Add("    Ctl.$property = d")
                $code.Add('    Exit Function')
                $code.Add('h:')
                $code.Add("    $id = `"err`" & Err.Number")
                $code.Add('    Err.Clear')
                $code.Add('    On Error Resume Next')
                $code.Add("    Ctl.$property = d")
                $code.Add('End Function')
                $code.Add('')
                $label = "$name | $property | set $value".Replace('"', "'")
                $run += "    If Want(`"$label`") Then Rec `"$label`", $id()"
            }
        }
    }
    $run += 'End Sub'
    $head = @(
        'VERSION 5.00',
        $objectLine,
        "Begin VB.Form $form",
        "   Caption         =   `"$name probe`"",
        '   ClientHeight    =   3000',
        '   ClientLeft      =   60',
        '   ClientTop       =   345',
        '   ClientWidth     =   5000',
        '   LinkTopic       =   "Form1"',
        '   ScaleHeight     =   3000',
        '   ScaleWidth      =   5000',
        '   StartUpPosition =   3  ''Windows Default',
        "   Begin $($choice.Namespace).$name Ctl",
        '      Height          =   1200',
        '      Left            =   120',
        '      TabIndex        =   0',
        '      Top             =   120',
        '      Width           =   4200',
        '      _ExtentX        =   7408',
        '      _ExtentY        =   2117',
        '      _Version        =   393216',
        '   End',
        'End',
        "Attribute VB_Name = `"$form`"",
        'Attribute VB_GlobalNameSpace = False',
        'Attribute VB_Creatable = False',
        'Attribute VB_PredeclaredId = True',
        'Attribute VB_Exposed = False',
        'Option Explicit',
        ''
    )
    [System.IO.File]::WriteAllText((Join-Path $Output "$form.frm"),
        (($head + $code.ToArray() + $run) -join $crlf) + $crlf, $utf)
}

$main = @"
Attribute VB_Name = "Probe"
Option Explicit


Private Seen As New Collection
Private Skipped As New Collection

Private Sub Load1(ByVal file As String, ByVal into As Collection)
    Dim n As Integer, t As String, k As Long
    If Dir(file) = "" Then Exit Sub
    n = FreeFile
    Open file For Input As #n
    Do While Not EOF(n)
        Line Input #n, t
        k = InStr(t, " = ")
        If k > 0 Then t = Left(t, k - 1)
        On Error Resume Next
        into.Add t, t
        On Error GoTo 0
    Loop
    Close #n
End Sub

Private Function Has(ByVal c As Collection, ByVal k As String) As Boolean
    Dim v As Variant
    On Error Resume Next
    v = c(k)
    Has = (Err.Number = 0)
End Function

Public Function Fmt(ByVal v As Variant) As String
    If IsNull(v) Then
        Fmt = "Null"
    Else
        Fmt = TypeName(v) & ":" & CStr(v)
    End If
End Function

Public Function Want(ByVal label As String) As Boolean
    Dim n As Integer
    Want = Not (Has(Seen, label) Or Has(Skipped, label))
    If Want Then
        n = FreeFile
        Open App.Path & "\current.txt" For Output As #n
        Print #n, label
        Close #n
    End If
End Function

Public Sub Rec(ByVal label As String, ByVal result As String)
    Dim n As Integer
    n = FreeFile
    Open App.Path & "\control-probe.txt" For Append As #n
    Print #n, label & " = " & result
    Close #n
End Sub

Public Sub Main()
    Load1 App.Path & "\control-probe.txt", Seen
    Load1 App.Path & "\skip.txt", Skipped
"@
$calls = foreach ($form in $forms) {
    "    Load $form"
    "    ${form}.Run"
    "    Unload $form"
}
$main = $main + $crlf + ($calls -join $crlf) + $crlf +
    'End Sub'
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $utf)

$project = @('Type=Exe', 'Startup="Sub Main"', 'Module=Probe; Probe.bas',
    "Object=$($choice.Guid)#$($choice.Version)#0; $($choice.Ocx)") +
    ($forms | ForEach-Object { "Form=$_.frm" }) +
    @(('Name="' + $choice.Name + '"'), 'Title="Control probe"')
[System.IO.File]::WriteAllText((Join-Path $Output ($choice.Name + '.vbp')),
    ($project -join $crlf) + $crlf, $utf)

Write-Host "Wrote $count probes to $Output"
