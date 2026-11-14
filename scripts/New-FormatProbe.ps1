<#
.SYNOPSIS
Generates the Format probe: a Visual Basic 6 project that records what
Format, FormatNumber, FormatCurrency, FormatPercent and FormatDateTime answer
for values of many kinds and pictures of every family.

.DESCRIPTION
Each probe is a function that sets a variable (typed, or a Variant that holds
a value) and returns what the call gives: "<TypeName>:<text>", "Null" or
"err<number>". Main writes one line per probe, "<call> | <value> = <result>",
to format-probe.txt next to the project. The pictures cover the numeric ones
(digit placeholders, thousands, percent, scientific, sections, literals), the
named ones (General Number, Currency, Fixed, Standard, Percent, Scientific,
Yes/No, True/False, On/Off), the text ones (@ & < > !), and the date and time
ones (the named formats and the letters of the picture).

.PARAMETER Output
The folder to write; the default is evidence\reference\format-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-FormatProbe.ps1
python scripts/run_vb6_probe.py `
    evidence/reference/format-probe/FormatProbe.vbp `
    evidence/reference/format-probe/format-probe.txt --timeout 900
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\format-probe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function New-Operand([string]$Name, [string]$Type, [string]$Value) {
    @{ Name = $Name; Type = $Type; Value = $Value }
}

$operands = @(
    (New-Operand 'Integer 0' 'Integer' '0'),
    (New-Operand 'Integer 5' 'Integer' '5'),
    (New-Operand 'Integer -5' 'Integer' '-5'),
    (New-Operand 'Long 1234567' 'Long' '1234567'),
    (New-Operand 'Single 0.5' 'Single' '0.5'),
    (New-Operand 'Double 1234.5678' 'Double' '1234.5678'),
    (New-Operand 'Double -1234.5678' 'Double' '-1234.5678'),
    (New-Operand 'Double 0.000123' 'Double' '0.000123'),
    (New-Operand 'Double 1E10' 'Double' '1E+10'),
    (New-Operand 'Double 0.5' 'Double' '0.5'),
    (New-Operand 'Double 2.5' 'Double' '2.5'),
    (New-Operand 'Double 3.5' 'Double' '3.5'),
    (New-Operand 'Currency 1234.5678' 'Currency' '1234.5678'),
    (New-Operand 'Byte 200' 'Byte' '200'),
    (New-Operand 'Boolean True' 'Boolean' 'True'),
    (New-Operand 'Boolean False' 'Boolean' 'False'),
    (New-Operand 'Date 2001-03-04 17:06:07' 'Date' '#3/4/2001 5:06:07 PM#'),
    (New-Operand 'Date 2000-01-01' 'Date' '#1/1/2000#'),
    (New-Operand 'Date time only' 'Date' '#9:05:00 AM#'),
    (New-Operand 'String "abc"' 'String' '"abc"'),
    (New-Operand 'String "123.456"' 'String' '"123.456"'),
    (New-Operand 'String ""' 'String' '""'),
    (New-Operand 'Variant Empty' 'Variant' ''),
    (New-Operand 'Variant Null' 'Variant' 'Null'),
    (New-Operand 'Variant Double 1234.5678' 'Variant' 'CDbl(1234.5678)'),
    (New-Operand 'Variant String "1234.5"' 'Variant' '"1234.5"'),
    (New-Operand 'Variant Date 2001-03-04' 'Variant' '#3/4/2001 5:06:07 PM#')
)

$pictures = @(
    '0', '00000', '0.00', '0.0000', '#', '#.##', '#,##0', '#,##0.00',
    '#,###', '0.0E+00', '0.00E-00', '0%', '0.00%', '#.#E+0',
    '$#,##0.00', '$#,##0.00;($#,##0.00)', '0.00;-0.00;Zero',
    '0;(0);"nil"', '+0;-0', '0.00 "units"', '\x0', '0\x', '*-0', '0_)',
    '#.00', '.00', '0.', '###-##', '(000) 000-0000',
    '@', '@@@@@', '&', '&&&', '!@@@@@', '<', '>', '<&&&', '>@@@@',
    'General Number', 'Currency', 'Fixed', 'Standard', 'Percent',
    'Scientific', 'Yes/No', 'True/False', 'On/Off',
    'General Date', 'Long Date', 'Medium Date', 'Short Date', 'Long Time',
    'Medium Time', 'Short Time',
    'yyyy-mm-dd', 'yy/m/d', 'dddd, mmmm d, yyyy', 'ddd mmm dd', 'd', 'dd',
    'ddddd', 'dddddd', 'm', 'mm', 'mmm', 'mmmm', 'yy', 'yyyy', 'y', 'q',
    'w', 'ww', 'h:nn:ss', 'hh:nn:ss AM/PM', 'h:n:s', 'h:mm AMPM', 'ttttt',
    'hh:mm:ss a/p', 'mm/dd/yyyy hh:nn', 'c', 'dd-mmm-yy h:nn', 'nn', 'ss'
)

$calls = @()
foreach ($p in $pictures) {
    $calls += ('Format({0}, "' + $p.Replace('"', '""') + '")')
}
$calls += @(
    'Format({0})', 'Format$({0}, "0.00")',
    'Format({0}, "0.00", 2)', 'Format({0}, "dddd", 2)',
    'Format({0}, "dddd", 1, 2)', 'Format({0}, "yyyy", 1, 2)',
    'Format({0}, "yyyy", 1, 3)',
    'FormatNumber({0})', 'FormatNumber({0}, 0)', 'FormatNumber({0}, 3)',
    'FormatNumber({0}, 2, True)', 'FormatNumber({0}, 2, False)',
    'FormatNumber({0}, 2, , True)', 'FormatNumber({0}, 2, , , True)',
    'FormatNumber({0}, 2, , , False)',
    'FormatNumber({0}, -1, True, True, True)',
    'FormatCurrency({0})', 'FormatCurrency({0}, 0)',
    'FormatCurrency({0}, 1, , True)', 'FormatCurrency({0}, 2, , , False)',
    'FormatPercent({0})', 'FormatPercent({0}, 0)', 'FormatPercent({0}, 3)',
    'FormatPercent({0}, 1, True)', 'FormatPercent({0}, 1, , True)',
    'FormatDateTime({0})', 'FormatDateTime({0}, 0)', 'FormatDateTime({0}, 1)',
    'FormatDateTime({0}, 2)', 'FormatDateTime({0}, 3)',
    'FormatDateTime({0}, 4)', 'FormatDateTime({0}, 5)'
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

# Four calls to a module (a module may not pass 32,767 lines).
$chunk = 0
for ($f = 0; $f -lt $calls.Count; $f += 4) {
    $moduleName = 'Fm{0:D2}' -f $chunk
    $chunk++
    $modules += $moduleName
    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("Attribute VB_Name = `"$moduleName`"")
    $lines.Add('Option Explicit')
    $lines.Add('')
    $run = @('Public Sub Run()')
    $last = [Math]::Min($f + 3, $calls.Count - 1)
    foreach ($template in $calls[$f..$last]) {
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

$main = @"
Attribute VB_Name = "Probe"
Option Explicit

Private Out As Integer

Public Sub Rec(ByVal label As String, ByVal result As String)
    Print #Out, label & " = " & result
End Sub

Public Sub Main()
    Out = FreeFile
    Open App.Path & "\format-probe.txt" For Output As #Out
$(($modules | ForEach-Object { "    $_.Run" }) -join $crlf)
    Close #Out
End Sub
"@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $utf)

$project = @('Type=Exe', 'Startup="Sub Main"', 'Module=Probe; Probe.bas') +
    ($modules | ForEach-Object { "Module=$_; $_.bas" }) +
    @('Name="FormatProbe"', 'Title="Format probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'FormatProbe.vbp'),
    ($project -join $crlf) + $crlf, $utf)

Write-Host "Wrote $count probes to $Output"
