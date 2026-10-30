<#
.SYNOPSIS
Installs or removes WFC (the Waughtal Form Compiler) and its libraries.

.DESCRIPTION
Run as an administrator the install is for every user of the machine and goes
to C:\wfc; run without administrator rights it is for the current user only
and goes to C:\wfc\users\<name>. -Scope overrides the choice.

Layout below the root:

  bin\                     wfc.exe, wfcl.exe, wfc-prompt.cmd (this machine's
                           architecture); bin\<arch>\ for every architecture
                           the package carries
  lib\<name>\<ver>\<arch>\ the libraries, each in its own versioned folder
                           (wfcrt, wfcvbrun, and wfcmscomctl, wfccomdlg32,
                           wfcmswinsck when present)
  install.json             what is installed, with versions
  Install-Wfc.ps1          this script, for the uninstall

The install puts bin on the PATH of the machine (or of the user), sets
WFC_HOME, adds a Start menu entry "WFC Command Prompt" (a console with wfc on
the PATH), a Windows Terminal profile, and an Apps & features entry.

.PARAMETER Source
The folder of the package: one folder per architecture (x64, x86, arm64)
holding wfc.exe, wfcl.exe, wfcrt.dll, wfcvbrun.dll and optionally
wfcmscomctl.dll, wfccomdlg32.dll and wfcmswinsck.dll.

.PARAMETER Scope
Machine (all users, needs administrator rights), User, or Auto (default).

.PARAMETER Root
Overrides the install folder.

.PARAMETER Uninstall
Removes what the install put there.

.PARAMETER RegisterMicrosoftIds
Also registers the controls under the Microsoft ProgIDs and class ids
(MSComctlLib.ListView and so on), so programs that ask for the Microsoft
control get the WFC library. What was registered before is kept and comes back
with the uninstall. Without it only WFC's own ProgIDs (WFC.MSComctlLib.*) are
registered.

.PARAMETER NoRegister
Leaves the libraries unregistered (they are registered with Windows by
default: HKLM for a machine install, HKCU for a user install).

.PARAMETER FilesOnly
Copies the files and writes install.json, but leaves the PATH, WFC_HOME, the
Start menu, Windows Terminal, Apps & features and the registration of the
libraries alone (for tests).

.EXAMPLE
powershell -NoProfile -File Install-Wfc.ps1 -Source .
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$Source = (Split-Path -Parent $PSCommandPath),
    [ValidateSet('Auto', 'Machine', 'User')]
    [string]$Scope = 'Auto',
    [string]$Root,
    [switch]$Uninstall,
    [switch]$RegisterMicrosoftIds,
    [switch]$NoRegister,
    [switch]$FilesOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot 'WfcInstallCommon.ps1')

function Add-PathEntry([string]$Entry, [string]$Target) {
    $current = [Environment]::GetEnvironmentVariable('Path', $Target)
    $parts = @($current -split ';' | Where-Object { $_ -ne '' })
    if ($parts -notcontains $Entry) {
        [Environment]::SetEnvironmentVariable(
            'Path', (($parts + $Entry) -join ';'), $Target)
    }
}

function Remove-PathEntry([string]$Entry, [string]$Target) {
    $current = [Environment]::GetEnvironmentVariable('Path', $Target)
    $parts = @($current -split ';' |
            Where-Object { $_ -ne '' -and $_ -ne $Entry })
    [Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), $Target)
}

$administrator = Test-Administrator
if ($Scope -eq 'Auto') {
    $Scope = if ($administrator) { 'Machine' } else { 'User' }
}
if ($Scope -eq 'Machine' -and -not $administrator -and -not $FilesOnly) {
    throw 'A machine-wide install needs an administrator; use -Scope User.'
}
$target = if ($Scope -eq 'Machine') { 'Machine' } else { 'User' }
if (-not $Root) {
    $Root = if ($Scope -eq 'Machine') {
        'C:\wfc'
    } else {
        "C:\wfc\users\$env:USERNAME"
    }
}
$bin = Join-Path $Root 'bin'
$prompt = Join-Path $bin 'wfc-prompt.cmd'
$menu = if ($Scope -eq 'Machine') {
    Join-Path $env:ProgramData 'Microsoft\Windows\Start Menu\Programs\WFC'
} else {
    Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\WFC'
}
$fragments = if ($Scope -eq 'Machine') {
    Join-Path $env:ProgramData 'Microsoft\Windows Terminal\Fragments\WFC'
} else {
    Join-Path $env:LOCALAPPDATA 'Microsoft\Windows Terminal\Fragments\WFC'
}
$uninstallKey = if ($Scope -eq 'Machine') {
    'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\WFC'
} else {
    'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\WFC'
}

# What the install adds outside the root: the PATH, WFC_HOME, a Start menu
# shortcut, a Windows Terminal profile and the Apps & features entry.
function Register-Integration([string]$Version) {
    Add-PathEntry $bin $target
    [Environment]::SetEnvironmentVariable('WFC_HOME', $Root, $target)

    New-Item -ItemType Directory -Force -Path $menu | Out-Null
    $shell = New-Object -ComObject WScript.Shell
    $link = $shell.CreateShortcut((Join-Path $menu 'WFC Command Prompt.lnk'))
    $link.TargetPath = "$env:SystemRoot\System32\cmd.exe"
    $link.Arguments = "/k `"$prompt`""
    $link.WorkingDirectory = $env:USERPROFILE
    $link.Description = 'A command prompt with wfc on the PATH'
    $link.Save()

    New-Item -ItemType Directory -Force -Path $fragments | Out-Null
    # The PowerShell profile has wfc on the PATH and tab completion for it.
    $session = "`$env:WFC_HOME='$Root'; `$env:Path='$bin;'+`$env:Path; " +
        'wfc completion powershell | Out-String | Invoke-Expression; ' +
        'wfc version'
    $profileJson = [ordered]@{
        profiles = @(
            [ordered]@{
                name = 'WFC'
                commandline = "cmd.exe /k `"$prompt`""
                startingDirectory = '%USERPROFILE%'
            }
            [ordered]@{
                name = 'WFC PowerShell'
                commandline = 'powershell.exe -NoLogo -NoExit -Command ' +
                    "`"$session`""
                startingDirectory = '%USERPROFILE%'
            }
        )
    } | ConvertTo-Json -Depth 4
    [IO.File]::WriteAllText((Join-Path $fragments 'wfc.json'), $profileJson,
        (New-Object Text.UTF8Encoding $false))

    $script = Join-Path $Root 'Install-Wfc.ps1'
    New-Item -Path $uninstallKey -Force | Out-Null
    $properties = @{
        DisplayName = 'WFC (Waughtal Form Compiler)'
        DisplayVersion = $Version
        Publisher = 'Waughtal'
        InstallLocation = $Root
        UninstallString = 'powershell -NoProfile -ExecutionPolicy Bypass ' +
            "-File `"$script`" -Uninstall -Scope $Scope"
        NoModify = 1
        NoRepair = 1
    }
    foreach ($key in $properties.Keys) {
        Set-ItemProperty -Path $uninstallKey -Name $key `
            -Value $properties[$key]
    }
}

# The DLLs of the libraries that register with Windows, from entries of
# "<library> <architecture>" names and version values (install.json).
function Get-RegisteredFile([object[]]$Entries) {
    $files = @()
    foreach ($entry in $Entries) {
        $library, $architecture = $entry.Name -split ' '
        if ($script:RegisteredLibraries -notcontains $library) { continue }
        $folder = "lib\$library\$($entry.Value)\$architecture"
        $files += [pscustomobject]@{
            Path = Join-Path $Root "$folder\$library.dll"
            Architecture = $architecture
        }
    }
    return $files
}

function Unregister-InstalledLibrary {
    $record = Join-Path $Root 'install.json'
    if (-not (Test-Path -LiteralPath $record)) { return }
    $data = Get-Content -Raw -LiteralPath $record | ConvertFrom-Json
    $entries = @($data.libraries.PSObject.Properties)
    foreach ($file in Get-RegisteredFile $entries) {
        if (Test-Path -LiteralPath $file.Path) {
            Invoke-LibraryRegistration -Dll $file.Path `
                -Architecture $file.Architecture -Scope $Scope -Unregister
        }
    }
}

if ($Uninstall) {
    if (-not $PSCmdlet.ShouldProcess($Root, 'Remove WFC')) { return }
    if (-not $FilesOnly -and -not $NoRegister) {
        Unregister-InstalledLibrary
    }
    if (-not $FilesOnly -and -not $NoRegister) {
        Unregister-InstalledLibrary
    }
    if (-not $FilesOnly) {
        Remove-PathEntry $bin $target
        [Environment]::SetEnvironmentVariable('WFC_HOME', $null, $target)
        foreach ($path in @($menu, $fragments, $uninstallKey)) {
            if (Test-Path -LiteralPath $path) {
                Remove-Item -LiteralPath $path -Recurse -Force
            }
        }
    }
    if (Test-Path -LiteralPath $Root) {
        # The running script lives in the root: leave it for the last.
        Get-ChildItem -LiteralPath $Root -Force |
            Where-Object { $_.Name -ne 'Install-Wfc.ps1' } |
            Remove-Item -Recurse -Force
        Write-Output "Removed WFC from $Root"
    }
    return
}

$architectures = @(
    'x64', 'x86', 'arm64' |
        Where-Object { Test-Path (Join-Path $Source "$_\wfc.exe") }
)
if ($architectures.Count -eq 0) {
    throw "No wfc.exe for any architecture below $Source."
}
$native = Get-NativeArchitecture
if ($architectures -notcontains $native) {
    $native = $architectures[0]
}
if (-not $PSCmdlet.ShouldProcess($Root, "Install WFC ($Scope)")) { return }

$libraries = [ordered]@{}
foreach ($architecture in $architectures) {
    $from = Join-Path $Source $architecture
    $archBin = Join-Path $bin $architecture
    New-Item -ItemType Directory -Force -Path $archBin | Out-Null
    foreach ($name in 'wfc.exe', 'wfcl.exe') {
        Copy-Item -LiteralPath (Join-Path $from $name) -Destination $archBin `
            -Force
    }
    foreach ($dll in 'wfcrt.dll', 'wfcvbrun.dll', 'wfcmscomctl.dll',
        'wfccomdlg32.dll', 'wfcmswinsck.dll') {
        $file = Join-Path $from $dll
        if (-not (Test-Path -LiteralPath $file)) { continue }
        $library = [IO.Path]::GetFileNameWithoutExtension($dll)
        $libraryVersion = Get-LibraryVersion $file
        $folder = Join-Path $Root "lib\$library\$libraryVersion\$architecture"
        New-Item -ItemType Directory -Force -Path $folder | Out-Null
        Copy-Item -LiteralPath $file -Destination $folder -Force
        $libraries["$library $architecture"] = $libraryVersion
    }
}
foreach ($name in 'wfc.exe', 'wfcl.exe') {
    Copy-Item -LiteralPath (Join-Path $bin "$native\$name") `
        -Destination $bin -Force
}
$version = '0.0.0'
if ($libraries.Contains("wfcrt $native")) {
    $version = $libraries["wfcrt $native"]
}
$promptText = @(
    '@echo off'
    'set "WFC_HOME=%~dp0.."'
    'set "PATH=%~dp0;%PATH%"'
    'title WFC command prompt'
    'prompt $E[36mwfc$E[0m $P$G'
    "echo WFC $version - type 'wfc help' for usage"
    'cmd /k'
)
Set-Content -LiteralPath $prompt -Value $promptText -Encoding ascii
if ($PSCommandPath -ne (Join-Path $Root 'Install-Wfc.ps1')) {
    # The uninstall runs from the root: it needs the shared helpers too.
    foreach ($name in 'Install-Wfc.ps1', 'WfcInstallCommon.ps1') {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) `
            -Destination (Join-Path $Root $name) -Force
    }
}
if (-not $FilesOnly) {
    Register-Integration $version
    if (-not $NoRegister) {
        $entries = @($libraries.GetEnumerator() | ForEach-Object {
                [pscustomobject]@{ Name = $_.Key; Value = $_.Value }
            })
        foreach ($file in Get-RegisteredFile $entries) {
            Invoke-LibraryRegistration -Dll $file.Path `
                -Architecture $file.Architecture -Scope $Scope `
                -MicrosoftIds:$RegisterMicrosoftIds
        }
    }
}

$record = [ordered]@{
    version = $version
    scope = $Scope
    root = $Root
    architectures = $architectures
    libraries = $libraries
}
# UTF-8 without a byte order mark, whatever the PowerShell edition.
[IO.File]::WriteAllText((Join-Path $Root 'install.json'),
    ($record | ConvertTo-Json -Depth 4), (New-Object Text.UTF8Encoding $false))
Write-Output "Installed WFC $version for $Scope at $Root"
