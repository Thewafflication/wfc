<#
.SYNOPSIS
Generates the arithmetic and logical operator probe: a Visual Basic 6 project
that records what `+ - * / \ Mod ^ & And Or Xor Eqv Imp` (and the unary `-` and
`Not`) answer for every pair of kinds of operands.

.DESCRIPTION
The comparison operators and Select Case were measured the same way (see
New-OperatorProbe.ps1). Each probe is a function that sets two variables (typed,
or Variants that hold a value) and returns what `a op b` gives: the type name
and the text of the result ("Integer:10"), "Null", or "err<number>". Main writes
one line per probe, "<a> | <op> | <b> = <result>", to arithmetic-probe.txt next
to the project.

.PARAMETER Output
The folder to write; the default is evidence\reference\arithmetic-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-ArithmeticProbe.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\arithmetic-probe')
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
    (New-Operand 'Integer 32767' 'Integer' '32767'),
    (New-Operand 'Long 5' 'Long' '5'),
    (New-Operand 'Long 2147483647' 'Long' '2147483647'),
    (New-Operand 'Single 0.1' 'Single' '0.1'),
    (New-Operand 'Double 5' 'Double' '5'),
    (New-Operand 'Double 5.5' 'Double' '5.5'),
    (New-Operand 'Double 2.5' 'Double' '2.5'),
    (New-Operand 'Double 0' 'Double' '0'),
    (New-Operand 'Currency 0.1' 'Currency' '0.1'),
    (New-Operand 'Byte 5' 'Byte' '5'),
    (New-Operand 'Byte 255' 'Byte' '255'),
    (New-Operand 'Boolean True' 'Boolean' 'True'),
    (New-Operand 'Boolean False' 'Boolean' 'False'),
    (New-Operand 'Date 2000-01-01' 'Date' '#1/1/2000#'),
    (New-Operand 'String "5"' 'String' '"5"'),
    (New-Operand 'String "abc"' 'String' '"abc"'),
    (New-Operand 'String ""' 'String' '""'),
    (New-Operand 'String "True"' 'String' '"True"'),
    (New-Operand 'Variant Empty' 'Variant' ''),
    (New-Operand 'Variant Null' 'Variant' 'Null'),
    (New-Operand 'Variant Integer 5' 'Variant' 'CInt(5)'),
    (New-Operand 'Variant Long 5' 'Variant' 'CLng(5)'),
    (New-Operand 'Variant Integer 32767' 'Variant' 'CInt(32767)'),
    (New-Operand 'Variant Double 5.5' 'Variant' 'CDbl(5.5)'),
    (New-Operand 'Variant Single 0.1' 'Variant' 'CSng(0.1)'),
    (New-Operand 'Variant Currency 0.1' 'Variant' 'CCur(0.1)'),
    (New-Operand 'Variant Byte 5' 'Variant' 'CByte(5)'),
    (New-Operand 'Variant String "5"' 'Variant' '"5"'),
    (New-Operand 'Variant String "abc"' 'Variant' '"abc"'),
    (New-Operand 'Variant String ""' 'Variant' '""'),
    (New-Operand 'Variant Boolean True' 'Variant' 'True'),
    (New-Operand 'Variant Boolean False' 'Variant' 'False'),
    (New-Operand 'Variant Date 2000-01-01' 'Variant' '#1/1/2000#')
)
$groups = @(
    @('+', '-', '*', '/'),
    @('\', 'Mod', '^', '&'),
    @('And', 'Or', 'Xor', 'Eqv', 'Imp')
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

function Add-Head($lines, [string]$Id, $A, $B) {
    $lines.Add("Public Function $Id() As String")
    $lines.Add("    Dim a As $($A.Type)")
    if ($B) {
        $lines.Add("    Dim b As $($B.Type)")
    }
    $lines.Add('    Dim r As Variant')
    if ($A.Value -ne '') {
        $lines.Add("    a = $($A.Value)")
    }
    if ($B -and $B.Value -ne '') {
        $lines.Add("    b = $($B.Value)")
    }
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null

$modules = @()
$count = 0
for ($g = 0; $g -lt $groups.Count; $g++) {
    for ($i = 0; $i -lt $operands.Count; $i++) {
        $a = $operands[$i]
        $moduleName = 'Ar{0}{1:D2}' -f $g, $i
        $modules += $moduleName
        $lines = [System.Collections.Generic.List[string]]::new()
        $lines.Add("Attribute VB_Name = `"$moduleName`"")
        $lines.Add('Option Explicit')
        $lines.Add('')
        $run = @('Public Sub Run()')
        foreach ($b in $operands) {
            foreach ($operator in $groups[$g]) {
                $count++
                $id = 'P{0:D5}' -f $count
                Add-Head $lines $id $a $b
                Add-Tail $lines $id "a $operator b"
                $label = "$($a.Name) | $operator | $($b.Name)"
                $label = $label.Replace('"', "'")
                $run += "    Rec `"$label`", $id()"
            }
        }
        $run += 'End Sub'
        Write-Module $moduleName ($lines.ToArray() + $run)
    }
}

# The unary operators.
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('Attribute VB_Name = "ArUnary"')
$lines.Add('Option Explicit')
$lines.Add('')
$run = @('Public Sub Run()')
foreach ($a in $operands) {
    foreach ($operator in @('-', 'Not ')) {
        $count++
        $id = 'P{0:D5}' -f $count
        Add-Head $lines $id $a $null
        Add-Tail $lines $id "${operator}a"
        $label = "$($operator.Trim()) | $($a.Name)".Replace('"', "'")
        $run += "    Rec `"$label`", $id()"
    }
}
$run += 'End Sub'
Write-Module 'ArUnary' ($lines.ToArray() + $run)
$modules += 'ArUnary'

$main = @"
Attribute VB_Name = "Probe"
Option Explicit

Private Out As Integer

Public Sub Rec(ByVal label As String, ByVal result As String)
    Print #Out, label & " = " & result
End Sub

Public Sub Main()
    Out = FreeFile
    Open App.Path & "\arithmetic-probe.txt" For Output As #Out
$(($modules | ForEach-Object { "    $_.Run" }) -join $crlf)
    Close #Out
End Sub
"@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $utf)

$project = @('Type=Exe', 'Startup="Sub Main"', 'Module=Probe; Probe.bas') +
    ($modules | ForEach-Object { "Module=$_; $_.bas" }) +
    @('Name="ArithmeticProbe"', 'Title="Arithmetic probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'ArithmeticProbe.vbp'),
    ($project -join $crlf) + $crlf, $utf)

Write-Host "Wrote $count probes to $Output"
