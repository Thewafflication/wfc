<#
.SYNOPSIS
Generates the statement probe: a Visual Basic 6 project that records what
small programs (statements, objects, errors) do.

.DESCRIPTION
Each probe is a function whose body is one snippet of Visual Basic. The snippet
reports through Say (a String) and SayV (a Variant: its type name and value);
the function returns everything that was said, or, when the snippet stops with
an error, what was said so far and "|err<number>:<description>". Main writes
one line per probe, "<name> = <result>", to statement-probe.txt next to the
project.

The snippets are in the here-string below, each introduced by a line "@@
<name>". A snippet may use the procedures of the module Helpers.

.PARAMETER Output
The folder to write; the default is evidence\reference\statement-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-StatementProbe.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\statement-probe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$snippetFile = Join-Path (Split-Path -Parent $PSScriptRoot) `
    'evidence\reference\statement-probe\snippets.txt'
if (-not (Test-Path $snippetFile)) {
    throw "The snippets are in $snippetFile"
}

$crlf = "`r`n"
$ascii = New-Object System.Text.ASCIIEncoding

function Write-Module([string]$Name, [string[]]$Lines) {
    $path = Join-Path $Output "$Name.bas"
    [System.IO.File]::WriteAllText($path, ($Lines -join $crlf) + $crlf, $ascii)
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null

# The snippets: "@@ name" starts one.
$snippets = New-Object System.Collections.Generic.List[object]
$current = $null
foreach ($line in (Get-Content -LiteralPath $snippetFile)) {
    if ($line.StartsWith('@@ ')) {
        $current = @{ Name = $line.Substring(3).Trim()
            Lines = New-Object System.Collections.Generic.List[string] }
        $snippets.Add($current)
    } elseif ($null -ne $current -and -not $line.StartsWith('#')) {
        $current.Lines.Add($line)
    }
}

# The classes and modules of the snippets: "## class Name" or "## module Name".
$extraFile = Join-Path (Split-Path -Parent $snippetFile) 'classes.txt'
$extras = New-Object System.Collections.Generic.List[object]
if (Test-Path $extraFile) {
    $currentExtra = $null
    foreach ($line in (Get-Content -LiteralPath $extraFile)) {
        if ($line -match '^## (class|module) (\w+)$') {
            $currentExtra = @{ Kind = $Matches[1]; Name = $Matches[2]
                Lines = New-Object System.Collections.Generic.List[string] }
            $extras.Add($currentExtra)
        } elseif ($null -ne $currentExtra -and -not $line.StartsWith('#')) {
            $currentExtra.Lines.Add($line)
        }
    }
}
foreach ($extra in $extras) {
    $header = @()
    if ($extra.Kind -eq 'class') {
        $header = @('VERSION 1.0 CLASS', 'BEGIN',
            "  MultiUse = -1  'True", 'END')
    }
    $header += "Attribute VB_Name = `"$($extra.Name)`""
    $extension = if ($extra.Kind -eq 'class') { 'cls' } else { 'bas' }
    $path = Join-Path $Output "$($extra.Name).$extension"
    [System.IO.File]::WriteAllText($path,
        (($header + $extra.Lines.ToArray()) -join $crlf) + $crlf, $ascii)
}

$helpers = @'
Attribute VB_Name = "Helpers"
Option Explicit

Public Acc As String

Public Sub Say(ByVal text As String)
    Acc = Acc & text & ";"
End Sub

Public Sub SayV(ByVal value As Variant)
    If IsNull(value) Then
        Acc = Acc & "Null;"
    ElseIf IsObject(value) Then
        Acc = Acc & "Object;"
    ElseIf IsArray(value) Then
        Acc = Acc & "Array;"
    Else
        Acc = Acc & TypeName(value) & ":" & CStr(value) & ";"
    End If
End Sub

Public Sub ByRefInc(ByRef n As Long)
    n = n + 1
End Sub

Public Sub ByValInc(ByVal n As Long)
    n = n + 1
End Sub

Public Function Opt(Optional ByVal a As Variant, _
        Optional b As Long = 7) As String
    If IsMissing(a) Then
        Opt = "missing," & b
    Else
        Opt = CStr(a) & "," & b
    End If
End Function

Public Function Total(ParamArray values() As Variant) As String
    Dim i As Long
    Dim sum As Double
    For i = LBound(values) To UBound(values)
        sum = sum + values(i)
    Next i
    Total = UBound(values) - LBound(values) + 1 & ":" & sum
End Function

Public Function Counter() As Long
    Static n As Long
    n = n + 1
    Counter = n
End Function

Public Function Fails(ByVal code As Long) As Long
    Err.Raise code
End Function

Public Function Twice(ByVal x As Long) As Long
    Twice = x * 2
End Function

Public Sub Swapper(ByRef p As Props)
    Set p = New Props
    p.Value = 99
End Sub

Public Sub Swap(ByRef a As Variant, ByRef b As Variant)
    Dim t As Variant
    t = a
    a = b
    b = t
End Sub
'@
Write-Module 'Helpers' ($helpers -split "`r?`n")

$modules = @()
$count = 0
$moduleIndex = 0
$lines = $null
$run = $null
$inModule = 0

function Start-Module {
    $script:moduleName = 'St{0:D2}' -f $script:moduleIndex
    $script:moduleIndex++
    $script:modules += $script:moduleName
    $script:lines = [System.Collections.Generic.List[string]]::new()
    $script:lines.Add("Attribute VB_Name = `"$($script:moduleName)`"")
    $script:lines.Add('Option Explicit')
    $script:lines.Add('')
    $script:run = @('Public Sub Run()', '    Dim r As String',
        '    On Error Resume Next')
    $script:inModule = 0
}

function Stop-Module {
    $script:run += 'End Sub'
    Write-Module $script:moduleName ($script:lines.ToArray() + $script:run)
}

Start-Module
foreach ($snippet in $snippets) {
    if ($script:inModule -ge 40) {
        Stop-Module
        Start-Module
    }
    $count++
    $script:inModule++
    $id = 'P{0:D4}' -f $count
    $lines.Add("Public Sub $id()")
    $lines.Add('    On Error GoTo h')
    foreach ($text in $snippet.Lines) {
        # a label or a line number starts in the first column
        if ($text -match '^[A-Za-z_][A-Za-z0-9_]*:$' -or $text -match '^\d+ ') {
            $lines.Add($text)
        } else {
            $lines.Add('    ' + $text)
        }
    }
    $lines.Add('    Exit Sub')
    $lines.Add('h:')
    $lines.Add('    Acc = Acc & "|err" & Err.Number & ":" & Err.Description')
    $lines.Add('End Sub')
    $lines.Add('')
    $run += '    Acc = ""'
    $run += '    Err.Clear'
    $run += "    $id"
    $run += '    r = Acc'
    $run += '    If Err.Number <> 0 Then'
    $run += '        r = Acc & "|propagated" & Err.Number & ":" & _'
    $run += '            Err.Description'
    $run += '    End If'
    $run += "    Rec `"$($snippet.Name)`", r"
}
Stop-Module

$main = @"
Attribute VB_Name = "Probe"
Option Explicit

Private Out As Integer

Public Sub Rec(ByVal label As String, ByVal result As String)
    Dim shown As String
    Dim i As Long
    Dim code As Long
    If Not (result Like "*[! -~]*") Then
        Print #Out, label & " = " & result
        Exit Sub
    End If
    For i = 1 To Len(result)
        code = AscW(Mid`$(result, i, 1))
        If code < 0 Then code = code + 65536
        If code < 32 Or code > 126 Then
            shown = shown & "<U+" & Right`$("0000" & Hex`$(code), 4) & ">"
        Else
            shown = shown & Mid`$(result, i, 1)
        End If
    Next i
    Print #Out, label & " = " & shown
End Sub

Public Sub Main()
    Out = FreeFile
    Open App.Path & "\statement-probe.txt" For Output As #Out
$(($modules | ForEach-Object { "    $_.Run" }) -join $crlf)
    Close #Out
End Sub
"@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $ascii)

$extraLines = @()
foreach ($extra in $extras) {
    if ($extra.Kind -eq 'class') {
        $extraLines += "Class=$($extra.Name); $($extra.Name).cls"
    } else {
        $extraLines += "Module=$($extra.Name); $($extra.Name).bas"
    }
}
$ole = 'Reference=*\G{00020430-0000-0000-C000-000000000046}#2.0#0#' +
    '..\..\Windows\SysWOW64\stdole2.tlb#OLE Automation'
$project = @('Type=Exe', $ole, 'Startup="Sub Main"', 'Module=Probe; Probe.bas',
    'Module=Helpers; Helpers.bas') + $extraLines +
    ($modules | ForEach-Object { "Module=$_; $_.bas" }) +
    @('Name="StatementProbe"', 'Title="Statement probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'StatementProbe.vbp'),
    ($project -join $crlf) + $crlf, $ascii)

Write-Host "Wrote $count probes to $Output"
