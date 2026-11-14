<#
.SYNOPSIS
Generates the case probe: a Visual Basic 6 project that records, for every
character whose UCase or LCase differs from itself, its code and the codes of
UCase and LCase.

.DESCRIPTION
Main writes one line per character, "<code> = <UCase> <LCase>" (hexadecimal),
to case-probe.txt next to the project. The reference is
evidence\reference\case-probe\vb6-reference.txt and scripts/New-CaseTable.py
turns it into the table that the interpreter uses.

.PARAMETER Output
The folder to write; the default is evidence\reference\case-probe.

.EXAMPLE
pwsh -NoProfile -File scripts/New-CaseProbe.ps1
#>
[CmdletBinding()]
param(
    [string]$Output = (Join-Path (Split-Path -Parent $PSScriptRoot) `
        'evidence\reference\case-probe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$crlf = "`r`n"
$ascii = New-Object System.Text.ASCIIEncoding
New-Item -ItemType Directory -Force -Path $Output | Out-Null

$main = @'
Attribute VB_Name = "Probe"
Option Explicit

Public Sub Main()
    Dim out As Integer
    Dim code As Long
    Dim ch As String
    Dim up As Long
    Dim low As Long
    out = FreeFile
    Open App.Path & "\case-probe.txt" For Output As #out
    For code = 1 To 65535
        If code < &HD800& Or code > &HDFFF& Then
            ch = ChrW(code)
            up = AscW(UCase(ch)) And &HFFFF&
            low = AscW(LCase(ch)) And &HFFFF&
            If up <> code Or low <> code Then
                Print #out, Hex(code) & " = " & Hex(up) & " " & Hex(low)
            End If
        End If
    Next code
    Close #out
End Sub
'@
[System.IO.File]::WriteAllText((Join-Path $Output 'Probe.bas'),
    ($main -replace "`r?`n", $crlf) + $crlf, $ascii)

$project = @('Type=Exe', 'Startup="Sub Main"', 'Module=Probe; Probe.bas',
    'Name="CaseProbe"', 'Title="Case probe"')
[System.IO.File]::WriteAllText((Join-Path $Output 'CaseProbe.vbp'),
    ($project -join $crlf) + $crlf, $ascii)

Write-Host "Wrote the case probe to $Output"
