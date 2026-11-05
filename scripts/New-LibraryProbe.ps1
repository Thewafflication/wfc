<#
.SYNOPSIS
Generates the library probe: a Visual Basic 6 project that records what the
functions of the VBA library answer for operands of every kind (the type and
text of the result, or the error number) and how the functions that compare
Strings follow `Option Compare` of the module that calls them.

.DESCRIPTION
Each probe is a function that sets a variable (typed, or a Variant that holds a
value) and returns what `f(a)` gives: "<TypeName>:<text>", "Null" or
"err<number>". Main writes one line per probe, "<f> | <a> = <result>", to
library-probe.txt next to the project. The modules LbBinary and LbText compare
the strings "Alpha", "alpha", ... with InStr, InStrRev, StrComp, Replace, Split
and Filter; LbText has `Option Compare Text`.

.PARAMETER Output
The folder to write; the default is evidence\reference\library-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-LibraryProbe.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\library-probe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function New-Operand([string]$Name, [string]$Type, [string]$Value) {
    @{ Name = $Name; Type = $Type; Value = $Value }
}

$operands = @(
    (New-Operand 'Integer 5' 'Integer' '5'),
    (New-Operand 'Integer 0' 'Integer' '0'),
    (New-Operand 'Integer -1' 'Integer' '-1'),
    (New-Operand 'Long 5' 'Long' '5'),
    (New-Operand 'Single 0.1' 'Single' '0.1'),
    (New-Operand 'Double 5.5' 'Double' '5.5'),
    (New-Operand 'Double 2.5' 'Double' '2.5'),
    (New-Operand 'Currency 0.1' 'Currency' '0.1'),
    (New-Operand 'Byte 5' 'Byte' '5'),
    (New-Operand 'Boolean True' 'Boolean' 'True'),
    (New-Operand 'Date 2000-01-01' 'Date' '#1/1/2000#'),
    (New-Operand 'String "5"' 'String' '"5"'),
    (New-Operand 'String "abc"' 'String' '"abc"'),
    (New-Operand 'String ""' 'String' '""'),
    (New-Operand 'Variant Empty' 'Variant' ''),
    (New-Operand 'Variant Null' 'Variant' 'Null'),
    (New-Operand 'Variant Integer 5' 'Variant' 'CInt(5)'),
    (New-Operand 'Variant Integer 0' 'Variant' 'CInt(0)'),
    (New-Operand 'Variant Long 5' 'Variant' 'CLng(5)'),
    (New-Operand 'Variant Double 5.5' 'Variant' 'CDbl(5.5)'),
    (New-Operand 'Variant Double 2.5' 'Variant' 'CDbl(2.5)'),
    (New-Operand 'Variant Single 0.1' 'Variant' 'CSng(0.1)'),
    (New-Operand 'Variant Currency 0.1' 'Variant' 'CCur(0.1)'),
    (New-Operand 'Variant Byte 5' 'Variant' 'CByte(5)'),
    (New-Operand 'Variant String "5"' 'Variant' '"5"'),
    (New-Operand 'Variant String "abc"' 'Variant' '"abc"'),
    (New-Operand 'Variant String ""' 'Variant' '""'),
    (New-Operand 'Variant Boolean True' 'Variant' 'True'),
    (New-Operand 'Variant Date 2000-01-01' 'Variant' '#1/1/2000#')
)

# One argument: {0} is the variable a.
$functions = @(
    'Len({0})', 'LenB({0})', 'UCase({0})', 'LCase({0})', 'UCase$({0})',
    'Trim({0})', 'LTrim({0})', 'RTrim({0})', 'Trim$({0})', 'Left({0}, 2)',
    'Left$({0}, 2)', 'Right({0}, 2)', 'Mid({0}, 2)', 'Mid({0}, 1, 1)',
    'Asc({0})', 'AscW({0})', 'Chr({0})', 'Chr$({0})', 'ChrW({0})', 'Space({0})',
    'String(3, {0})', 'StrReverse({0})', 'Abs({0})', 'Int({0})', 'Fix({0})',
    'Sgn({0})', 'Sqr({0})', 'Exp({0})', 'Log({0})', 'Cos({0})',
    'Atn({0})', 'Hex({0})', 'Oct({0})', 'Hex$({0})', 'Str({0})', 'Str$({0})',
    'Val({0})', 'CStr({0})', 'CInt({0})', 'CLng({0})', 'CDbl({0})',
    'CSng({0})', 'CCur({0})', 'CBool({0})', 'CByte({0})', 'CDate({0})',
    'CVar({0})', 'IsNumeric({0})', 'IsDate({0})', 'IsNull({0})',
    'IsEmpty({0})', 'VarType({0})', 'TypeName({0})', 'Format({0})',
    'Format({0}, "0.00")', 'Round({0})', 'Round({0}, 1)', 'Left({0}, "1")',
    'Mid({0}, "1", 2)', 'Year({0})', 'Month({0})', 'Day({0})', 'Hour({0})',
    'DateAdd("d", 1, {0})', 'DateDiff("d", {0}, #1/2/2000#)',
    'DateSerial({0}, 1, 1)', 'Weekday({0})', 'FormatNumber({0})',
    'FormatCurrency({0})', 'FormatPercent({0})', 'Choose({0}, "x", "y")',
    'Switch({0}, "x")', 'IIf({0}, "x", "y")', 'Array({0})(0)',
    'Join(Array({0}), ",")', 'Replace({0}, "a", "b")', 'InStr({0}, "a")',
    'StrComp({0}, "a")', 'Space$({0})', 'LCase$({0})', 'Len({0} & "x")'
)

# Two Strings, to tell how Option Compare is followed.
$words = @('Alpha', 'alpha', 'ALPHA', 'a', 'A', 'pha', 'PHA', '')
$pairs = @(
    'InStr({0}, {1})', 'InStr(2, {0}, {1})', 'InStr(1, {0}, {1}, 0)',
    'InStr(1, {0}, {1}, 1)', 'InStrRev({0}, {1})', 'StrComp({0}, {1})',
    'StrComp({0}, {1}, 0)', 'StrComp({0}, {1}, 1)', 'Replace({0}, {1}, "x")',
    'UBound(Split({0}, {1}))', 'UBound(Filter(Array({0}), {1}))',
    '({0} = {1})', '({0} < {1})', '({0} Like {1})'
)

$crlf = "`r`n"
$utf = New-Object System.Text.ASCIIEncoding

function Write-Module([string]$Name, [string[]]$Lines) {
    $path = Join-Path $Output "$Name.bas"
    [System.IO.File]::WriteAllText($path, ($Lines -join $crlf) + $crlf, $utf)
}

function Add-Tail($lines, [string]$Id, [string]$Expression) {
    $lines.Add('    On Error GoTo h')
    $lines.Add("    r = $Expression")
    $lines.Add('    If IsNull(r) Then')
    $lines.Add("        $Id = `"Null`"")
    $lines.Add('    Else')
    $lines.Add("        $Id = TypeName(r) & `":`" & CStr(r)")
    $lines.Add('    End If')
    $lines.Add('    Exit Function')
    $lines.Add('h:')
    $lines.Add("    $Id = `"err`" & Err.Number")
    $lines.Add('End Function')
    $lines.Add('')
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null

$modules = @()
$count = 0

# The library functions against the operands, six functions to a module.
$chunk = 0
for ($f = 0; $f -lt $functions.Count; $f += 6) {
    $moduleName = 'Lb{0:D2}' -f $chunk
    $chunk++
    $modules += $moduleName
    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("Attribute VB_Name = `"$moduleName`"")
    $lines.Add('Option Explicit')
    $lines.Add('')
    $run = @('Public Sub Run()')
    $last = [Math]::Min($f + 5, $functions.Count - 1)
    foreach ($template in $functions[$f..$last]) {
        foreach ($a in $operands) {
            $count++
            $id = 'P{0:D5}' -f $count
            $lines.Add("Public Function $id() As String")
            $lines.Add("    Dim a As $($a.Type)")
            $lines.Add('    Dim r As Variant')
            if ($a.Value -ne '') {
                $lines.Add("    a = $($a.Value)")
            }
            Add-Tail $lines $id ($template -f 'a')
            $label = ("$template | $($a.Name)" -f 'a').Replace('"', "'")
            $run += "    Rec `"$label`", $id()"
        }
    }
    $run += 'End Sub'
    Write-Module $moduleName ($lines.ToArray() + $run)
}

# Option Compare: one module with the option and one without.
foreach ($mode in @('Binary', 'Text')) {
    $moduleName = "Lb$mode"
    $modules += $moduleName
    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("Attribute VB_Name = `"$moduleName`"")
    if ($mode -eq 'Text') {
        $lines.Add('Option Compare Text')
    }
    $lines.Add('Option Explicit')
    $lines.Add('')
    $run = @('Public Sub Run()')
    foreach ($kind in @('String', 'Variant')) {
        foreach ($template in $pairs) {
            foreach ($x in $words) {
                foreach ($y in $words) {
                    $count++
                    $id = 'P{0:D5}' -f $count
                    $lines.Add("Public Function $id() As String")
                    $lines.Add("    Dim a As $kind")
                    $lines.Add("    Dim b As $kind")
                    $lines.Add('    Dim r As Variant')
                    $lines.Add("    a = `"$x`"")
                    $lines.Add("    b = `"$y`"")
                    Add-Tail $lines $id ($template -f 'a', 'b')
                    $label = "$mode $kind | $template | '$x' | '$y'"
                    $label = $label -f 'a', 'b'
                    $label = $label.Replace('"', "'")
                    $run += "    Rec `"$label`", $id()"
                }
            }
        }
    }
    $run += 'End Sub'
    Write-Module $moduleName ($lines.ToArray() + $run)
}

$main = @"
Attribute VB_Name = "Probe"
Option Explicit

Private Out As Integer

Public Sub Rec(ByVal label As String, ByVal result As String)
    Print #Out, label & " = " & result
End Sub

Public Sub Main()
    Out = FreeFile
    Open App.Path & "\library-probe.txt" For Output As #Out
$(($modules | ForEach-Object { "    $_.Run" }) -join $crlf)
    Close #Out
End Sub
"@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $utf)

$project = @('Type=Exe', 'Startup="Sub Main"', 'Module=Probe; Probe.bas') +
    ($modules | ForEach-Object { "Module=$_; $_.bas" }) +
    @('Name="LibraryProbe"', 'Title="Library probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'LibraryProbe.vbp'),
    ($project -join $crlf) + $crlf, $utf)

Write-Host "Wrote $count probes to $Output"
