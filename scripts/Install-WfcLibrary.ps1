<#
.SYNOPSIS
Installs or removes one WFC library (wfcvbrun, wfcmscomctl, wfccomdlg32 or
wfcmswinsck) on its own, and registers it with Windows.

.DESCRIPTION
The library goes to <root>\lib\<name>\<version>\<arch>, beside the other
versions, where a dynamically linked program finds the newest one: installing a
newer package updates the programs without touching wfc. The root is the one
of the WFC installation (WFC_HOME, else C:\wfc for an administrator and
C:\wfc\users\<name> otherwise). The DLL is registered with Windows
(regsvr32): HKLM for a machine install, HKCU for a user install. install.json
of the installation records the library and its version.

.PARAMETER Source
The folder of the package: one folder per architecture (x64, x86, arm64)
holding the DLL.

.PARAMETER Library
The library to install or remove (for example wfcmscomctl); found from the
DLLs in Source when omitted.

.PARAMETER Scope
Machine (all users, needs administrator rights), User, or Auto (default).

.PARAMETER Root
Overrides the installation folder.

.PARAMETER Uninstall
Unregisters the library and removes the versions in Source from the root.

.PARAMETER RegisterMicrosoftIds
Also registers the controls under the Microsoft ProgIDs and class ids. What
was registered before is kept and comes back with the uninstall.

.PARAMETER NoRegister
Copies the files and records the library but does not register it.

.PARAMETER FilesOnly
Copies the files only (for tests): no registration and no install.json.

.EXAMPLE
powershell -NoProfile -File Install-WfcLibrary.ps1 -Source .
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$Source = (Split-Path -Parent $PSCommandPath),
    [string]$Library,
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

$administrator = Test-Administrator
if ($Scope -eq 'Auto') {
    $Scope = if ($administrator) { 'Machine' } else { 'User' }
}
if ($Scope -eq 'Machine' -and -not $administrator -and -not $FilesOnly) {
    throw 'A machine-wide install needs an administrator; use -Scope User.'
}
if (-not $Root) {
    $Root = $env:WFC_HOME
}
if (-not $Root) {
    $Root = if ($Scope -eq 'Machine') {
        'C:\wfc'
    } else {
        "C:\wfc\users\$env:USERNAME"
    }
}

$architectures = @(
    'x64', 'x86', 'arm64' |
        Where-Object { Test-Path (Join-Path $Source $_) }
)
if (-not $Library) {
    $found = @(
        $script:RegisteredLibraries |
            Where-Object {
                $name = $_
                $architectures | Where-Object {
                    Test-Path (Join-Path $Source "$_\$name.dll")
                }
            }
    )
    if ($found.Count -ne 1) {
        throw "Name the library with -Library (found: $($found -join ', '))."
    }
    $Library = $found[0]
}
if ($script:RegisteredLibraries -notcontains $Library) {
    throw "Unknown library '$Library'."
}

$recordPath = Join-Path $Root 'install.json'

function Read-Record {
    if (Test-Path -LiteralPath $recordPath) {
        return Get-Content -Raw -LiteralPath $recordPath | ConvertFrom-Json
    }
    return $null
}

# Writes install.json with the library set to the version (or removed when the
# version is empty). UTF-8 without a byte order mark.
function Update-Record([string]$Architecture, [string]$Version) {
    $record = Read-Record
    $libraries = [ordered]@{}
    $scopeName = $Scope
    $architectureList = @($Architecture)
    $recorded = '0.0.0'
    if ($record) {
        foreach ($property in $record.libraries.PSObject.Properties) {
            $libraries[$property.Name] = $property.Value
        }
        $scopeName = $record.scope
        $architectureList = @($record.architectures) + @($Architecture) |
            Select-Object -Unique
        $recorded = $record.version
    }
    $key = "$Library $Architecture"
    if ($Version) {
        $libraries[$key] = $Version
    } else {
        $libraries.Remove($key)
    }
    $updated = [ordered]@{
        version = $recorded
        scope = $scopeName
        root = $Root
        architectures = @($architectureList)
        libraries = $libraries
    }
    [IO.File]::WriteAllText($recordPath,
        ($updated | ConvertTo-Json -Depth 4),
        (New-Object Text.UTF8Encoding $false))
}

if ($Uninstall) {
    if (-not $PSCmdlet.ShouldProcess($Root, "Remove $Library")) { return }
    foreach ($architecture in $architectures) {
        $dll = Join-Path $Source "$architecture\$Library.dll"
        if (-not (Test-Path -LiteralPath $dll)) { continue }
        $version = Get-LibraryVersion $dll
        $folder = Join-Path $Root "lib\$Library\$version\$architecture"
        $installed = Join-Path $folder "$Library.dll"
        if (-not $FilesOnly -and -not $NoRegister -and
            (Test-Path -LiteralPath $installed)) {
            Invoke-LibraryRegistration -Dll $installed `
                -Architecture $architecture -Scope $Scope -Unregister
        }
        if (Test-Path -LiteralPath $folder) {
            Remove-Item -LiteralPath $folder -Recurse -Force
        }
        if (-not $FilesOnly -and (Test-Path -LiteralPath $recordPath)) {
            Update-Record $architecture ''
        }
    }
    Write-Output "Removed $Library from $Root"
    return
}

if ($architectures.Count -eq 0) {
    throw "No architecture folder below $Source."
}
if (-not $PSCmdlet.ShouldProcess($Root, "Install $Library ($Scope)")) { return }
foreach ($architecture in $architectures) {
    $dll = Join-Path $Source "$architecture\$Library.dll"
    if (-not (Test-Path -LiteralPath $dll)) { continue }
    $version = Get-LibraryVersion $dll
    $folder = Join-Path $Root "lib\$Library\$version\$architecture"
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    Copy-Item -LiteralPath $dll -Destination $folder -Force
    if ($FilesOnly) { continue }
    if (-not $NoRegister) {
        Invoke-LibraryRegistration -Dll (Join-Path $folder "$Library.dll") `
            -Architecture $architecture -Scope $Scope `
            -MicrosoftIds:$RegisterMicrosoftIds
    }
    New-Item -ItemType Directory -Force -Path $Root | Out-Null
    Update-Record $architecture $version
    Write-Output "Installed $Library $version ($architecture) at $folder"
}
