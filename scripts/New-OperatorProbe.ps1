<#
.SYNOPSIS
Generates the comparison operator probe: a Visual Basic 6 project that
records what `=`, `<` and `>` answer for every pair of kinds of operands.

.DESCRIPTION
Select Case was measured the same way (see New-SelectCaseProbe.ps1). Each probe
is a function that sets two variables (typed, or Variants that hold a value)
and returns what `a op b` gives: "True", "False", "Null", or "err<number>".
Main writes one line per probe, "<a> | <op> | <b> = <result>", to
operator-probe.txt next to the project.

Run the project under Visual Basic 6, then under WFC, and compare the files.

.PARAMETER Output
The folder to write; the default is evidence\reference\operator-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-OperatorProbe.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\operator-probe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# An operand: its name, the type it is declared with and the value it is given
# (none for a Variant that stays Empty).
function New-Operand([string]$Name, [string]$Type, [string]$Value) {
    @{ Name = $Name; Type = $Type; Value = $Value }
}

$operands = @(
    (New-Operand 'Integer 5' 'Integer' '5'),
    (New-Operand 'Integer -1' 'Integer' '-1'),
    (New-Operand 'Long 5' 'Long' '5'),
    (New-Operand 'Long 70000' 'Long' '70000'),
    (New-Operand 'Single 5' 'Single' '5'),
    (New-Operand 'Single 0.1' 'Single' '0.1'),
    (New-Operand 'Double 5' 'Double' '5'),
    (New-Operand 'Double 0.1' 'Double' '0.1'),
    (New-Operand 'Double 5.4' 'Double' '5.4'),
    (New-Operand 'Currency 5' 'Currency' '5'),
    (New-Operand 'Currency 0.1' 'Currency' '0.1'),
    (New-Operand 'Byte 5' 'Byte' '5'),
    (New-Operand 'Boolean True' 'Boolean' 'True'),
    (New-Operand 'Boolean False' 'Boolean' 'False'),
    (New-Operand 'Date 2000-01-01' 'Date' '#1/1/2000#'),
    (New-Operand 'String "5"' 'String' '"5"'),
    (New-Operand 'String "abc"' 'String' '"abc"'),
    (New-Operand 'String ""' 'String' '""'),
    (New-Operand 'String "10"' 'String' '"10"'),
    (New-Operand 'String "9"' 'String' '"9"'),
    (New-Operand 'String "True"' 'String' '"True"'),
    (New-Operand 'Variant Empty' 'Variant' ''),
    (New-Operand 'Variant Null' 'Variant' 'Null'),
    (New-Operand 'Variant Integer 5' 'Variant' 'CInt(5)'),
    (New-Operand 'Variant Long 5' 'Variant' 'CLng(5)'),
    (New-Operand 'Variant Double 5' 'Variant' 'CDbl(5)'),
    (New-Operand 'Variant Double 0.1' 'Variant' 'CDbl(0.1)'),
    (New-Operand 'Variant Single 0.1' 'Variant' 'CSng(0.1)'),
    (New-Operand 'Variant Currency 0.1' 'Variant' 'CCur(0.1)'),
    (New-Operand 'Variant Byte 5' 'Variant' 'CByte(5)'),
    (New-Operand 'Variant String "5"' 'Variant' '"5"'),
    (New-Operand 'Variant String "abc"' 'Variant' '"abc"'),
    (New-Operand 'Variant String ""' 'Variant' '""'),
    (New-Operand 'Variant String "10"' 'Variant' '"10"'),
    (New-Operand 'Variant String "9"' 'Variant' '"9"'),
    (New-Operand 'Variant Boolean True' 'Variant' 'True'),
    (New-Operand 'Variant Boolean False' 'Variant' 'False'),
    (New-Operand 'Variant Date 2000-01-01' 'Variant' '#1/1/2000#')
)
$operators = @('=', '<', '>')
$crlf = "`r`n"
$utf = New-Object System.Text.ASCIIEncoding

function Write-Module([string]$Name, [string[]]$Lines) {
    $path = Join-Path $Output "$Name.bas"
    [System.IO.File]::WriteAllText($path, ($Lines -join $crlf) + $crlf, $utf)
}

function New-Probe([string]$Id, $A, $B, [string]$Operator) {
    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("Public Function $Id() As String")
    $lines.Add("    Dim a As $($A.Type)")
    $lines.Add("    Dim b As $($B.Type)")
    $lines.Add('    Dim r As Variant')
    if ($A.Value -ne '') {
        $lines.Add("    a = $($A.Value)")
    }
    if ($B.Value -ne '') {
        $lines.Add("    b = $($B.Value)")
    }
    $lines.Add('    On Error GoTo h')
    $lines.Add("    r = a $Operator b")
    $lines.Add('    If IsNull(r) Then')
    $lines.Add("        $Id = `"Null`"")
    $lines.Add('    Else')
    $lines.Add("        $Id = CStr(r)")
    $lines.Add('    End If')
    $lines.Add('    Exit Function')
    $lines.Add('h:')
    $lines.Add("    $Id = `"err`" & Err.Number")
    $lines.Add('End Function')
    $lines.Add('')
    return , $lines.ToArray()
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null

# One module for each left operand: a module holds about 120 functions.
function New-Wave([string]$Prefix, [string]$Letter, [bool]$Text) {
    $modules = @()
    $count = 0
    foreach ($a in $operands) {
        $isString = $a.Type -eq 'String' -or $a.Name -like 'Variant String*'
        if ($Text -and -not $isString) {
            continue
        }
        $index = $operands.IndexOf($a)
        $moduleName = '{0}{1:D2}' -f $Prefix, $index
        $modules += $moduleName
        $header = @("Attribute VB_Name = `"$moduleName`"")
        if ($Text) {
            $header += 'Option Compare Text'
        }
        $lines = $header + @('Option Explicit', '')
        $run = @('Public Sub Run()')
        foreach ($b in $operands) {
            $other = $b.Type -eq 'String' -or $b.Name -like 'Variant String*'
            if ($Text -and -not $other) {
                continue
            }
            foreach ($operator in $operators) {
                $count++
                $id = '{0}{1:D5}' -f $Letter, $count
                $lines += New-Probe $id $a $b $operator
                $label = "$($a.Name) | $operator | $($b.Name)"
                $label = $label.Replace('"', "'")
                $run += "    Rec `"$label`", $id()"
            }
        }
        $run += 'End Sub'
        Write-Module $moduleName ($lines + $run)
    }
    return $modules
}

$modules = @()
$modules += New-Wave 'OpA' 'A' $false
$modules += New-Wave 'OpT' 'T' $true

$main = @"
Attribute VB_Name = "Probe"
Option Explicit

Private Out As Integer

Public Sub Rec(ByVal label As String, ByVal result As String)
    Print #Out, label & " = " & result
End Sub

Public Sub Main()
    Out = FreeFile
    Open App.Path & "\operator-probe.txt" For Output As #Out
$(($modules | ForEach-Object { "    $_.Run" }) -join $crlf)
    Close #Out
End Sub
"@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $utf)

$project = @('Type=Exe', 'Startup="Sub Main"', 'Module=Probe; Probe.bas') +
    ($modules | ForEach-Object { "Module=$_; $_.bas" }) +
    @('Name="OperatorProbe"', 'Title="Operator probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'OperatorProbe.vbp'),
    ($project -join $crlf) + $crlf, $utf)

Write-Host "Wrote the probe to $Output"
