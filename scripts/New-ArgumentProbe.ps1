<#
.SYNOPSIS
Generates the argument probe: a Visual Basic 6 project that records what the
functions of the VBA library with several arguments answer for edge values
(zero, negative, fractional, Null, empty, longer than the text, in the middle
of a number).

.DESCRIPTION
Each probe is a function that returns what a call gives: "<TypeName>:<text>",
"Null" or "err<number>". The arguments are Visual Basic expressions (literals,
so each keeps its own type, and Null). Main writes one line per probe,
"<call> = <result>", to argument-probe.txt next to the project.

.PARAMETER Output
The folder to write; the default is evidence\reference\argument-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-ArgumentProbe.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\argument-probe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$texts = @('"Hello"', '""', 'Null', '12345', '"a b c"')
$counts = @('0', '1', '2', '5', '6', '-1', '1.5', '2.5', 'Null', '"2"',
    '100000')
$starts = @('0', '1', '2', '5', '6', '-1', '1.5', 'Null', '"2"')

$groups = New-Object System.Collections.Generic.List[object]
function Add-Group([string]$Template, [object[]]$Lists) {
    $groups.Add(@{ Template = $Template; Lists = $Lists })
}

# Text slicing.
Add-Group 'Mid({0}, {1})' @($texts, $starts)
Add-Group 'Mid({0}, {1}, {2})' @($texts, $starts, $counts)
Add-Group 'Left({0}, {1})' @($texts, $counts)
Add-Group 'Right({0}, {1})' @($texts, $counts)
Add-Group 'Space({0})' @(, $counts)
Add-Group 'String({0}, "ab")' @(, $counts)
Add-Group 'String({0}, 65)' @(, $counts)
Add-Group 'Chr({0})' @(, @('0', '65', '255', '256', '-1', '65.5', '66.5',
    'Null', '"65"', '65536'))
Add-Group 'ChrW({0})' @(, @('0', '65', '255', '256', '-1', '65.5', '66.5',
    'Null', '"65"', '65536', '-32768', '32768'))
Add-Group 'Asc({0})' @(, @('"A"', '"ABC"', '""', 'Null', '65',
    ('"' + [char]0x00E9 + '"'), ('"' + [char]0x20AC + '"')))

# Searching.
$hay = @('"Hello world"', '""', 'Null', '"aaa"')
$needle = @('"o"', '"l"', '""', 'Null', '"O"', '"world"', '"zz"')
Add-Group 'InStr({0}, {1})' @($hay, $needle)
Add-Group 'InStr({2}, {0}, {1})' @($hay, $needle, @('1', '2', '5', '12', '0',
    '-1', 'Null'))
Add-Group 'InStr(1, {0}, {1}, {2})' @($hay, $needle, @('0', '1', '2', 'Null',
    '3'))
Add-Group 'InStrRev({0}, {1})' @($hay, $needle)
Add-Group 'InStrRev({0}, {1}, {2})' @($hay, $needle, @('1', '5', '11', '12',
    '0', '-1', '-2'))
Add-Group 'Replace({0}, {1}, "X")' @($hay, $needle)
Add-Group 'Replace({0}, "l", "X", {1})' @($hay, @('1', '2', '4', '12', '0',
    '-1', 'Null'))
Add-Group 'Replace({0}, "l", "X", 1, {1})' @($hay, @('-1', '0', '1', '2', '5',
    'Null', '-2'))
Add-Group 'Replace({0}, "L", "X", 1, -1, {1})' @($hay, @('0', '1', '2'))
Add-Group 'UBound(Split({0}, {1}))' @(@('"a,b,c"', '""', 'Null', '"a,,b"',
    '",a,"'), @('","', '""', '"x"', 'Null'))
Add-Group 'UBound(Split("a,b,c,d", ",", {0}))' @(, @('-1', '0', '1', '2', '10',
    'Null'))
Add-Group 'Join(Split("a,b,c", ","), {0})' @(, @('"-"', '""', 'Null', '1'))
Add-Group 'StrComp({0}, {1})' @(@('"a"', '"A"', '""', 'Null', '"b"'), @('"a"',
    '"A"', '""', 'Null', '"b"'))

# Conversions of text and numbers.
$numbers = @('"12abc"', '"  12"', '"12  "', '"&H1F"', '"&O17"', '"&HFFFF"',
    '"&HFFFFFFFF"', '"1e3"', '"1d3"', '"1,5"', '"-.5"', '"+5"', '"12 34"',
    '"1E"', '"."', '"-"', '"&"', '"0x10"', '"2.5"', '"3.5"', '"-2.5"',
    '"32767.5"', '"32768"', '"1e10"', '"abc"', '""', '"True"', '"$5"', '"(5)"',
    '"1.5.2"', '"1 000"', '"1,000"', '"&H"', '" &H10 "', '"1e+"', '"4e-2"')
Add-Group 'Val({0})' @(, $numbers)
Add-Group 'CInt({0})' @(, $numbers)
Add-Group 'CLng({0})' @(, $numbers)
Add-Group 'CDbl({0})' @(, $numbers)
Add-Group 'CCur({0})' @(, $numbers)
Add-Group 'CBool({0})' @(, $numbers)
Add-Group 'CByte({0})' @(, $numbers)
Add-Group 'CSng({0})' @(, $numbers)
Add-Group 'IsNumeric({0})' @(, $numbers)
$rounding = @('0.5', '1.5', '2.5', '3.5', '-0.5', '-1.5', '-2.5', '0.49999999',
    '32767.5', '32768', '-32768.5', '2147483647.5', '2147483648', '255.5',
    '256', '-0.4', '1E+20', '0.000001', '1234.5678')
Add-Group 'CInt({0})' @(, $rounding)
Add-Group 'CLng({0})' @(, $rounding)
Add-Group 'CByte({0})' @(, $rounding)
Add-Group 'CCur({0})' @(, $rounding)
Add-Group 'CStr({0})' @(, $rounding)
Add-Group 'Int({0})' @(, $rounding)
Add-Group 'Fix({0})' @(, $rounding)
Add-Group 'Round({0})' @(, $rounding)
Add-Group 'Round({0}, 1)' @(, $rounding)
Add-Group 'Round({0}, 2)' @(, $rounding)
Add-Group 'Round(2.345, {0})' @(, @('-1', '0', '1', '2', '3', '10', '15',
    'Null', '1.5'))
Add-Group 'Hex({0})' @(, @('0', '255', '-1', '-255', '65535', '65536',
    '2147483647',
    '-2147483648', '255.5', '256.5', 'Null', '"255"', '"abc"', '4294967295',
    '1E10'))
Add-Group 'Oct({0})' @(, @('0', '8', '-1', '-8', '65535', '2147483647',
    '-2147483648', '7.5', '8.5', 'Null', '"8"', '4294967295'))
Add-Group 'Str({0})' @(, $rounding)
Add-Group 'Sgn({0})' @(, @('0', '-0.0001', '5', '"5"', 'Null', '"x"',
    '-2147483648'))
Add-Group 'Abs({0})' @(, @('-5', '-32768', '-2147483648', '-0.5', '"-5"',
    'Null', '"x"', '-128'))
Add-Group 'Sqr({0})' @(, @('0', '4', '2', '-1', '"4"', 'Null', '"x"', '1E300'))
Add-Group 'Log({0})' @(, @('1', '0', '-1', '2.718281828459045', '"1"', 'Null',
    '10'))
Add-Group 'Exp({0})' @(, @('0', '1', '-1', '709', '710', '"1"', 'Null', '-745'))
Add-Group 'Atn({0})' @(, @('0', '1', '-1', '1E300', '"1"', 'Null'))
Add-Group 'CDate({0})' @(, @('"1/2/2000"', '"2000-01-02"', '"January 5, 2000"',
    '"5 Jan 2000"', '"12:30"', '"12:30:45 PM"', '"1/2/2000 5:06 PM"',
    '"13/1/2000"',
    '"2/30/2000"', '"abc"', '""', '36526', '36526.5', '0', '-1', '"2000"',
    '"1/2"',
    '"Jan 2"', '"1/1/100"', '"1/1/99"', '"1/1/30"', '"1/1/29"', '"12/31/9999"',
    '"1/1/10000"', 'Null', 'True', '2958465', '2958466', '-657434', '-657435'))
Add-Group 'IsDate({0})' @(, @('"1/2/2000"', '"abc"', '""', '36526', 'Null',
    '"12:30"',
    '"2/30/2000"', '"13/1/2000"', '"1/1/100"', '"Jan 2"', '"2000-01-02"',
    'True'))

# Dates.
$dates = @('#1/31/2000#', '#2/29/2000#', '#12/31/1999 11:59:59 PM#',
    '#1/1/1900#',
    '#12/30/1899#', '#3/1/2001 6:30:00 AM#', '"1/31/2000"', 'Null', '36526.75')
Add-Group 'DateAdd("m", 1, {0})' @(, $dates)
Add-Group 'DateAdd("yyyy", 1, {0})' @(, $dates)
Add-Group 'DateAdd("d", -1, {0})' @(, $dates)
Add-Group 'DateAdd("h", 25, {0})' @(, $dates)
Add-Group 'DateAdd("q", 1, {0})' @(, $dates)
Add-Group 'DateAdd("ww", 1, {0})' @(, $dates)
Add-Group 'DateAdd("{1}", 1, {0})' @($dates[0..2], @('yyyy', 'q', 'm', 'y',
    'd', 'w', 'ww', 'h', 'n', 's', 'x', 'D', ''))
Add-Group 'DateAdd("m", {0}, #1/31/2000#)' @(, @('1', '-1', '12', '-13', '0.5',
    '1.5', '2.5', 'Null', '"1"', '100000', '-100000'))
Add-Group 'DateDiff("{2}", {0}, {1})' @(@('#1/31/2000#', '#12/31/1999#',
    '#2/29/2000 11:59:59 PM#'),
    @('#2/1/2000#', '#1/1/2001#', '#12/31/1999 11:59:59 PM#', '#3/1/2000#'),
    @('yyyy', 'q', 'm', 'y', 'd', 'w', 'ww', 'h', 'n', 's'))
Add-Group 'DatePart("{1}", {0})' @(@('#1/1/2000#', '#12/31/2000#',
    '#1/1/1999#', '#12/31/1999#', '#3/4/2001 5:06:07 PM#', 'Null'),
    @('yyyy', 'q', 'm', 'y', 'd', 'w', 'ww', 'h', 'n', 's'))
Add-Group 'DatePart("ww", {0}, {1}, {2})' @(@('#1/1/2000#', '#12/31/2000#',
    '#1/1/1999#', '#1/3/2000#'),
    @('1', '2', '3', '7', '0', '8'), @('1', '2', '3', '0', '4'))
Add-Group 'Weekday({0}, {1})' @(@('#1/1/2000#', '#1/2/2000#', '#12/30/1899#'),
    @('0', '1', '2', '7', '8', 'Null'))
Add-Group 'DateSerial({0}, {1}, {2})' @(@('2000', '1999', '99', '0', '30',
    '100', '1899', '9999', '10000'),
    @('1', '12', '13', '0', '-1', '2'), @('1', '31', '32', '0', '-1', '29'))
Add-Group 'TimeSerial({0}, {1}, {2})' @(@('0', '12', '24', '25', '-1'), @('0',
    '30', '60', '-1'), @('0', '59', '60', '-1'))
Add-Group 'DateValue({0})' @(, @('"1/2/2000"', '"abc"', '"12:30"',
    '"1/2/2000 5 PM"', 'Null', '#1/2/2000 5:00 PM#', '""'))
Add-Group 'TimeValue({0})' @(, @('"1/2/2000"', '"abc"', '"12:30"', '"5 PM"',
    '"13:61"', 'Null', '#1/2/2000 5:00 PM#', '""'))
Add-Group 'MonthName({0})' @(, @('0', '1', '12', '13', '1.5', 'Null', '"3"'))
Add-Group 'MonthName({0}, True)' @(, @('1', '12'))
Add-Group 'WeekdayName({0}, {1}, {2})' @(@('0', '1', '7', '8', 'Null'),
    @('True', 'False'), @('0', '1', '2', '7', '8'))

# Pattern matching.
$subjects = @('"abc"', '"ABC"', '"a"', '""', '"a*c"', '"a?c"', '"a#c"',
    '"a1c"', '"[abc]"',
    '"a-c"', '"abcabc"', '"123"', '"a]c"', '"a[c"')
$patterns = @('"abc"', '"a*"', '"*c"', '"a?c"', '"a#c"', '"[a-c]*"',
    '"[!a-c]*"',
    '"[abc][abc][abc]"', '"*"', '"?"', '""', '"a[*]c"', '"a[?]c"', '"a[#]c"',
    '"[[]*"', '"*[]]*"', '"[a-]"', '"[]"', '"[a"', '"#"', '"###"', '"a*b*c"',
    '"[c-a]*"', '"[A-C]*"', '"*[!a]*"', '"a[!]]c"', '"ab" & "c"', 'Null')
Add-Group '({0} Like {1})' @($subjects, $patterns)

$conversions = @('vbUpperCase', 'vbLowerCase', 'vbProperCase', 'vbWide',
    'vbNarrow',
    'vbUnicode', 'vbFromUnicode', '0', '3', '99')
Add-Group 'StrConv({0}, {1})' @(@('"hello wORLD o''neil"', '""', 'Null',
    '"abc def"', '12'), $conversions)
Add-Group 'UCase({0})' @(, @(('"stra' + [char]0x00DF + 'e"'),
    ('"' + [char]0x00E9 + 'a"'), '"i"', ('"' + [char]0x01C6 + '"')))
Add-Group 'LCase({0})' @(, @('"STRASSE"', ('"' + [char]0x00C9 + 'A"'),
    ('"' + [char]0x0130 + '"'), ('"' + [char]0x03A3 + '"')))
Add-Group 'Trim({0})' @(, @('"  a  "', '" "', ('"' + [char]9 + 'a"'),
    ('"a' + [char]160 + '"'), 'Null', '12', '""'))
Add-Group 'Filter(Array("a","B","ab"), {0}, {1}, {2})' @(@('"a"', '"A"', '""',
    'Null'),
    @('True', 'False'), @('0', '1'))
Add-Group 'LTrim$({0})' @(, @('"  a"', 'Null', '5'))
Add-Group 'Len({0})' @(, @('"abc"', '""', 'Null', ('"' + [char]0x00E9 + '"')))
Add-Group 'StrReverse({0})' @(, @('"abc"', '""', 'Null', '123', '"a b"'))
Add-Group 'LenB({0})' @(, @('"abc"', '""', ('"' + [char]0x00E9 + '"'),
    ('"' + [char]0x20AC + '"')))
Add-Group 'AscB({0})' @(, @('"A"', ('"' + [char]0x00E9 + '"'),
    ('"' + [char]0x20AC + '"'), '""'))
Add-Group 'AscW({0})' @(, @('"A"', ('"' + [char]0x00E9 + '"'),
    ('"' + [char]0x20AC + '"'), '""', '"AB"', 'Null'))
Add-Group 'Hex$({0})' @(, @('255', 'Null', '-1'))
Add-Group 'Format$({0})' @(, @('Null', '5', '"x"'))

$crlf = "`r`n"
$utf = New-Object System.Text.ASCIIEncoding

function Write-Module([string]$Name, [string[]]$Lines) {
    $path = Join-Path $Output "$Name.bas"
    [System.IO.File]::WriteAllText($path, ($Lines -join $crlf) + $crlf, $utf)
}

function Get-Combination([object[]]$Lists) {
    $results = New-Object System.Collections.Generic.List[object]
    $results.Add(@())
    foreach ($list in $Lists) {
        $next = New-Object System.Collections.Generic.List[object]
        foreach ($prefix in $results) {
            foreach ($item in $list) {
                $next.Add(@($prefix) + @($item))
            }
        }
        $results = $next
    }
    return , $results
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null

# Non-ASCII text is written as the characters Visual Basic reads from ANSI
# source (code page 1252): ChrW keeps the file plain ASCII.
function ConvertTo-Vb([string]$Text) {
    $builder = New-Object System.Text.StringBuilder
    foreach ($character in $Text.ToCharArray()) {
        if ([int]$character -gt 126 -or [int]$character -lt 32) {
            [void]$builder.Append('" & ChrW(' + [int]$character + ') & "')
        } else {
            [void]$builder.Append($character)
        }
    }
    $builder.ToString().Replace('"" & ', '').Replace(' & ""', '')
}

$modules = @()
$count = 0
$moduleIndex = 0
$moduleLines = $null
$moduleRun = $null
$moduleProbes = 0

function Start-Module {
    $script:moduleName = 'Ar{0:D2}' -f $script:moduleIndex
    $script:moduleIndex++
    $script:modules += $script:moduleName
    $script:moduleLines = [System.Collections.Generic.List[string]]::new()
    $script:moduleLines.Add("Attribute VB_Name = `"$($script:moduleName)`"")
    $script:moduleLines.Add('Option Explicit')
    $script:moduleLines.Add('')
    $script:moduleRun = @('Public Sub Run()')
    $script:moduleProbes = 0
}

function Stop-Module {
    $script:moduleRun += 'End Sub'
    Write-Module $script:moduleName `
        ($script:moduleLines.ToArray() + $script:moduleRun)
}

Start-Module
foreach ($group in $groups) {
    foreach ($combination in (Get-Combination $group.Lists)) {
        if ($script:moduleProbes -ge 80) {
            Stop-Module
            Start-Module
        }
        $count++
        $script:moduleProbes++
        $id = 'P{0:D5}' -f $count
        $expression = $group.Template
        for ($i = 0; $i -lt $combination.Count; $i++) {
            $expression = $expression.Replace("{$i}", [string]$combination[$i])
        }
        $moduleLines.Add("Public Function $id() As String")
        $moduleLines.Add('    Dim r As Variant')
        $moduleLines.Add('    On Error GoTo h')
        $moduleLines.Add('    r = ' + (ConvertTo-Vb $expression))
        $moduleLines.Add('    If IsNull(r) Then')
        $moduleLines.Add("        $id = `"Null`"")
        $moduleLines.Add('    Else')
        $moduleLines.Add("        $id = TypeName(r) & `":`" & CStr(r)")
        $moduleLines.Add('    End If')
        $moduleLines.Add('    Exit Function')
        $moduleLines.Add('h:')
        $moduleLines.Add("    $id = `"err`" & Err.Number")
        $moduleLines.Add('End Function')
        $moduleLines.Add('')
        $label = $expression.Replace('"', "'")
        $label = ($label.ToCharArray() | ForEach-Object {
            if ([int]$_ -gt 126 -or [int]$_ -lt 32) {
                '<U+{0:X4}>' -f [int]$_
            } else {
                $_
            }
        }) -join ''
        $moduleRun += "    Rec `"$($label.Replace('"', '""'))`", $id()"
    }
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
    Open App.Path & "\argument-probe.txt" For Output As #Out
$(($modules | ForEach-Object { "    $_.Run" }) -join $crlf)
    Close #Out
End Sub
"@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $utf)

$project = @('Type=Exe', 'Startup="Sub Main"', 'Module=Probe; Probe.bas') +
    ($modules | ForEach-Object { "Module=$_; $_.bas" }) +
    @('Name="ArgumentProbe"', 'Title="Argument probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'ArgumentProbe.vbp'),
    ($project -join $crlf) + $crlf, $utf)

Write-Host "Wrote $count probes to $Output"
