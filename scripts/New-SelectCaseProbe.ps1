<#
.SYNOPSIS
Generates the Select Case probe: a Visual Basic 6 project that records how
Select Case compares every kind of selector with every kind of Case value.

.DESCRIPTION
WFC must behave like Visual Basic 6 here, and the documentation does not say
enough (for example, how a Variant that holds a number compares with a String
constant). The probe settles it by measurement: run it under Visual Basic 6,
then under WFC, and compare the two output files.

The project is written to evidence\reference\select-case-probe. Each probe is a
function with one Select Case of one selector against one Case clause; it
returns "match", "nomatch" or "err<number>". Main writes one line per probe,
"<selector> | <form> | <value> = <result>", to select-case-probe.txt next to the
project.

.PARAMETER Output
The folder to write; the default is evidence\reference\select-case-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-SelectCaseProbe.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\select-case-probe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# A selector: its name, the Dim line and the lines that give it its value.
$selectors = @(
    @{ Name = 'Integer 5'; Setup = @('Dim s As Integer', 's = 5') },
    @{ Name = 'Long 5'; Setup = @('Dim s As Long', 's = 5') },
    @{ Name = 'Single 5'; Setup = @('Dim s As Single', 's = 5') },
    @{ Name = 'Double 5'; Setup = @('Dim s As Double', 's = 5') },
    @{ Name = 'Currency 5'; Setup = @('Dim s As Currency', 's = 5') },
    @{ Name = 'Byte 5'; Setup = @('Dim s As Byte', 's = 5') },
    @{ Name = 'Boolean True'; Setup = @('Dim s As Boolean', 's = True') },
    @{ Name = 'Date 2000-01-01'; Setup = @('Dim s As Date', 's = #1/1/2000#') },
    @{ Name = 'String "5"'; Setup = @('Dim s As String', 's = "5"') },
    @{ Name = 'String "abc"'; Setup = @('Dim s As String', 's = "abc"') },
    @{ Name = 'String ""'; Setup = @('Dim s As String', 's = ""') },
    @{ Name = 'Variant Empty'; Setup = @('Dim s As Variant') },
    @{ Name = 'Variant Null'; Setup = @('Dim s As Variant', 's = Null') },
    @{ Name = 'Variant Integer 5';
        Setup = @('Dim s As Variant', 's = CInt(5)') },
    @{ Name = 'Variant Long 5'; Setup = @('Dim s As Variant', 's = CLng(5)') },
    @{ Name = 'Variant Double 5';
        Setup = @('Dim s As Variant', 's = CDbl(5)') },
    @{ Name = 'Variant String "5"'; Setup = @('Dim s As Variant', 's = "5"') },
    @{ Name = 'Variant String "abc"';
        Setup = @('Dim s As Variant', 's = "abc"') },
    @{ Name = 'Variant Boolean True';
        Setup = @('Dim s As Variant', 's = True') },
    @{ Name = 'Variant Date 2000-01-01';
        Setup = @('Dim s As Variant', 's = #1/1/2000#') }
)

# A Case value: its name and the expression. The G* names are module-level
# Variants set by Init in Probe.bas.
$values = @(
    @{ Name = 'Integer 5'; Expr = '5' },
    @{ Name = 'Long 5'; Expr = '5&' },
    @{ Name = 'Single 5'; Expr = '5!' },
    @{ Name = 'Double 5'; Expr = '5#' },
    @{ Name = 'Currency 5'; Expr = '5@' },
    @{ Name = 'Integer -1'; Expr = '-1' },
    @{ Name = 'Integer 0'; Expr = '0' },
    @{ Name = 'Boolean True'; Expr = 'True' },
    @{ Name = 'Boolean False'; Expr = 'False' },
    @{ Name = 'Date 2000-01-01'; Expr = '#1/1/2000#' },
    @{ Name = 'String "5"'; Expr = '"5"' },
    @{ Name = 'String "abc"'; Expr = '"abc"' },
    @{ Name = 'String ""'; Expr = '""' },
    @{ Name = 'Null'; Expr = 'Null' },
    @{ Name = 'Empty'; Expr = 'Empty' },
    @{ Name = 'Variant Integer 5'; Expr = 'GInt5' },
    @{ Name = 'Variant String "5"'; Expr = 'GStr5' },
    @{ Name = 'Variant String "abc"'; Expr = 'GAbc' },
    @{ Name = 'Variant Null'; Expr = 'GNull' },
    @{ Name = 'Variant Empty'; Expr = 'GEmpty' }
)

# The four clause forms: how a value is written in a Case.
$forms = @(
    @{ Key = 'Eq'; Name = 'Case v'; Clause = { param($v) $v } },
    @{ Key = 'Lt'; Name = 'Case Is < v'; Clause = { param($v) "Is < $v" } },
    @{ Key = 'Gt'; Name = 'Case Is > v'; Clause = { param($v) "Is > $v" } },
    @{ Key = 'Range'; Name = 'Case v To v'; Clause = { param($v) "$v To $v" } }
)

$crlf = "`r`n"
$utf = New-Object System.Text.ASCIIEncoding

function Write-Module([string]$Name, [string[]]$Lines) {
    $path = Join-Path $Output "$Name.bas"
    [System.IO.File]::WriteAllText($path, ($Lines -join $crlf) + $crlf, $utf)
}

function New-Probe([string]$Id, [string[]]$Setup, [string]$Clause) {
    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("Public Function $Id() As String")
    foreach ($line in $Setup) {
        $lines.Add("    $line")
    }
    $lines.Add('    On Error GoTo h')
    $lines.Add('    Select Case s')
    $lines.Add("        Case $Clause")
    $lines.Add("            $Id = `"match`"")
    $lines.Add('            Exit Function')
    $lines.Add('    End Select')
    $lines.Add("    $Id = `"nomatch`"")
    $lines.Add('    Exit Function')
    $lines.Add('h:')
    $lines.Add("    $Id = `"err`" & Err.Number")
    $lines.Add('End Function')
    $lines.Add('')
    return , $lines.ToArray()
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null

$modules = @()
foreach ($form in $forms) {
    $moduleName = "Probe$($form.Key)"
    $modules += $moduleName
    $lines = @("Attribute VB_Name = `"$moduleName`"", 'Option Explicit', '')
    $run = @('Public Sub Run()')
    $count = 0
    foreach ($selector in $selectors) {
        foreach ($value in $values) {
            $count++
            $id = 'P{0:D4}' -f $count
            $clause = & $form.Clause $value.Expr
            $lines += New-Probe $id $selector.Setup $clause
            $label = "$($selector.Name) | $($form.Name) | $($value.Name)"
            $run += "    Rec `"$($label.Replace('"', "'"))`", $id()"
        }
    }
    $run += 'End Sub'
    Write-Module $moduleName ($lines + $run)
}

# Option Compare Text: only strings change.
$textSelectors = @(
    @{ Name = 'String "abc"'; Setup = @('Dim s As String', 's = "abc"') },
    @{ Name = 'String "B"'; Setup = @('Dim s As String', 's = "B"') },
    @{ Name = 'String "10"'; Setup = @('Dim s As String', 's = "10"') },
    @{ Name = 'Variant String "abc"';
        Setup = @('Dim s As Variant', 's = "abc"') }
)
$textValues = @(
    @{ Name = 'String "ABC"'; Expr = '"ABC"' },
    @{ Name = 'String "a"'; Expr = '"a"' },
    @{ Name = 'String "c"'; Expr = '"c"' },
    @{ Name = 'String "9"'; Expr = '"9"' },
    @{ Name = 'String "A"'; Expr = '"A"' }
)
$lines = @(
    'Attribute VB_Name = "ProbeText"', 'Option Compare Text',
    'Option Explicit', ''
)
$run = @('Public Sub Run()')
$count = 0
foreach ($form in $forms) {
    foreach ($selector in $textSelectors) {
        foreach ($value in $textValues) {
            $count++
            $id = 'T{0:D4}' -f $count
            $lines += New-Probe $id $selector.Setup (& $form.Clause $value.Expr)
            $label = "Option Compare Text | $($selector.Name) | " +
                "$($form.Name) | $($value.Name)"
            $run += "    Rec `"$($label.Replace('"', "'"))`", $id()"
        }
    }
}
$run += 'End Sub'
Write-Module 'ProbeText' ($lines + $run)
$modules += 'ProbeText'

# The flow of a Select Case: the order in which expressions are evaluated and
# when the evaluation stops.
$flow = @'
Attribute VB_Name = "ProbeFlow"
Option Explicit

Private Calls As String

Private Function F(ByVal n As Long) As Long
    Calls = Calls & n
    F = n
End Function

Private Function Boom() As Long
    Calls = Calls & "B"
    Boom = 1 / 0
End Function

Private Function Fail(ByVal e As Long) As String
    Fail = "err" & e
End Function

Public Sub Run()
    Dim r As String, n As Long, v As Variant

    Calls = "": r = "nomatch"
    Select Case F(2)
        Case F(1), F(2), F(3): r = "match"
        Case F(4): r = "later"
    End Select
    Rec "flow | selector once, list stops at the first match", _
        r & " calls=" & Calls

    Calls = "": r = "nomatch"
    Select Case F(9)
        Case F(1) To F(3), F(9): r = "match"
    End Select
    Rec "flow | range bounds evaluated, then the next value", _
        r & " calls=" & Calls

    Calls = "": r = "nomatch"
    Select Case 5
        Case 5: r = "first"
        Case Boom(): r = "never"
    End Select
    Rec "flow | a later Case is not evaluated after a match", _
        r & " calls=" & Calls

    Calls = "": r = "nomatch"
    Select Case 5
        Case 5, Boom(): r = "first"
    End Select
    Rec "flow | a later value of the list is not evaluated", _
        r & " calls=" & Calls

    Calls = "": r = "nomatch"
    On Error Resume Next
    Select Case 5
        Case 4, Boom(): r = "never"
        Case Else: r = "else"
    End Select
    Rec "flow | an error in a value that is reached", _
        r & " err=" & Err.Number & " calls=" & Calls
    Err.Clear
    On Error GoTo 0

    v = "abc": r = "nomatch"
    On Error Resume Next
    Select Case v
        Case "abc", 5: r = "match"
    End Select
    Rec "flow | list: match before an uncomparable value", _
        r & " err=" & Err.Number
    Err.Clear
    r = "nomatch"
    Select Case v
        Case 5, "abc": r = "match"
    End Select
    Rec "flow | list: uncomparable value before the match", _
        r & " err=" & Err.Number
    Err.Clear
    On Error GoTo 0

    n = 5: r = "nomatch"
    Select Case True
        Case n > 9: r = "gt9"
        Case n > 3: r = "gt3"
    End Select
    Rec "flow | Select Case True with conditions", r

    n = 5: r = "nomatch"
    Select Case n > 3
        Case True: r = "true"
    End Select
    Rec "flow | selector is a comparison", r

    r = "nomatch"
    Select Case 5
        Case 10 To 1: r = "backwards"
    End Select
    Rec "flow | a range written backwards", r

    r = "nomatch"
    Select Case 5
        Case 1 To 3, 4 To 6, 99: r = "second range"
    End Select
    Rec "flow | ranges and values mixed", r

    r = "nomatch"
    Select Case 5
        Case Is > 9, Is > 4: r = "second Is"
    End Select
    Rec "flow | Is clauses mixed", r

    r = "nomatch"
    Select Case 5
        Case 5: r = "first"
        Case 5: r = "second"
        Case Else: r = "else"
    End Select
    Rec "flow | only the first matching Case runs", r

    r = "nomatch"
    Select Case 6
        Case 5: r = "five"
        Case Else: r = "else"
    End Select
    Rec "flow | Case Else", r

    r = "nomatch"
    Select Case 1
        Case 1
            Select Case 2
                Case 2: r = "inner"
            End Select
    End Select
    Rec "flow | nested", r
End Sub
'@
[System.IO.File]::WriteAllText((Join-Path $Output 'ProbeFlow.bas'),
    ($flow -replace "`r?`n", $crlf) + $crlf, $utf)
$modules += 'ProbeFlow'

$main = @"
Attribute VB_Name = "Probe"
Option Explicit

Public GInt5 As Variant
Public GStr5 As Variant
Public GAbc As Variant
Public GNull As Variant
Public GEmpty As Variant

Private Out As Integer

Public Sub Rec(ByVal label As String, ByVal result As String)
    Print #Out, label & " = " & result
End Sub

Public Sub Main()
    GInt5 = CInt(5)
    GStr5 = "5"
    GAbc = "abc"
    GNull = Null
    Out = FreeFile
    Open App.Path & "\select-case-probe.txt" For Output As #Out
$(($modules | ForEach-Object { "    $_.Run" }) -join $crlf)
    Close #Out
End Sub
"@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $utf)

$project = @("Type=Exe", 'Startup="Sub Main"', 'Module=Probe; Probe.bas') +
    ($modules | ForEach-Object { "Module=$_; $_.bas" }) +
    @('Name="SelectCaseProbe"', 'Title="Select Case probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'SelectCaseProbe.vbp'),
    ($project -join $crlf) + $crlf, $utf)

Write-Host "Wrote the probe to $Output"
