<#
.SYNOPSIS
Small always-on-top window that follows a ctest log and shows progress.

.DESCRIPTION
Tails the log file written by `ctest` (lines such as `12/377 Test  #12: name
... Passed`) and shows a progress bar, the pass/fail counts and the last lines
of the log. Start it before the run, or run the tests through it:

  pwsh scripts/Show-TestProgress.ps1 -Run

.PARAMETER Log
The log file to follow (default: out/test-progress.log).

.PARAMETER Run
Run ctest for the x64 debug build into the log and follow it.

.PARAMETER Filter
A ctest -R expression when -Run is given.
#>
[CmdletBinding()]
param(
    [string]$Log = (Join-Path $PSScriptRoot '..\out\test-progress.log'),
    [switch]$Run,
    [string]$Filter = '',
    [string]$BuildDir = (Join-Path $PSScriptRoot `
        '..\out\build\windows-x64-debug')
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$Log = [IO.Path]::GetFullPath($Log)
New-Item -ItemType Directory -Force -Path (Split-Path $Log) | Out-Null
$process = $null
if ($Run) {
    Set-Content -Path $Log -Value '' -Encoding ascii
    $arguments = @('--test-dir', $BuildDir, '-C', 'Debug',
        '--output-on-failure')
    if ($Filter) { $arguments += @('-R', $Filter) }
    $process = Start-Process ctest -ArgumentList $arguments -NoNewWindow `
        -PassThru `
        -RedirectStandardOutput $Log -RedirectStandardError "$Log.err"
}
elseif (-not (Test-Path $Log)) {
    Set-Content -Path $Log -Value '' -Encoding ascii
}

$form = New-Object Windows.Forms.Form
$form.Text = 'WFC tests'
$form.TopMost = $true
$form.Size = New-Object Drawing.Size(460, 300)
$form.StartPosition = 'Manual'
$area = [Windows.Forms.Screen]::PrimaryScreen.WorkingArea
$form.Location = New-Object Drawing.Point(($area.Right - 470), ($area.Top + 10))
$form.FormBorderStyle = 'SizableToolWindow'

$status = New-Object Windows.Forms.Label
$status.Dock = 'Top'
$status.Height = 22
$status.Text = 'waiting for tests...'
$bar = New-Object Windows.Forms.ProgressBar
$bar.Dock = 'Top'
$bar.Height = 20
$bar.Minimum = 0
$bar.Maximum = 100
$box = New-Object Windows.Forms.TextBox
$box.Multiline = $true
$box.ReadOnly = $true
$box.ScrollBars = 'Vertical'
$box.Dock = 'Fill'
$box.Font = New-Object Drawing.Font('Consolas', 8)
$form.Controls.Add($box)
$form.Controls.Add($bar)
$form.Controls.Add($status)

$state = @{ Offset = 0L; Done = 0; Total = 0; Failed = 0; Text = '' }
$timer = New-Object Windows.Forms.Timer
$timer.Interval = 500
$timer.Add_Tick({
    $stream = $null
    try {
        $stream = [IO.File]::Open($Log, 'Open', 'Read', 'ReadWrite')
        if ($stream.Length -lt $state.Offset) {
            $state.Offset = 0
            $state.Text = ''
        }
        [void]$stream.Seek($state.Offset, 'Begin')
        $reader = New-Object IO.StreamReader($stream)
        $chunk = $reader.ReadToEnd()
        $state.Offset = $stream.Position
    }
    catch { return }
    finally { if ($stream) { $stream.Dispose() } }
    if (-not $chunk) { return }
    foreach ($line in ($chunk -split "`r?`n")) {
        if ($line -match '^\s*(\d+)/(\d+) Test\s+#\d+:') {
            $state.Done = [int]$Matches[1]
            $state.Total = [int]$Matches[2]
            if ($line -match '\*\*\*|Failed') { $state.Failed++ }
        }
    }
    $state.Text += $chunk
    if ($state.Text.Length -gt 20000) {
        $state.Text = $state.Text.Substring($state.Text.Length - 20000)
    }
    $box.Text = $state.Text
    $box.SelectionStart = $box.TextLength
    $box.ScrollToCaret()
    if ($state.Total -gt 0) {
        $bar.Value = [Math]::Min(100, [int](100 * $state.Done / $state.Total))
        $status.Text = "$($state.Done) / $($state.Total) tests, " +
            "$($state.Failed) failed"
    }
    if ($state.Text -match 'Total Test time') {
        $status.Text += ' - finished'
    }
})
$timer.Start()
$form.Add_FormClosed({ $timer.Stop() })
[void]$form.ShowDialog()
if ($process -and -not $process.HasExited) { $process.Kill() }
