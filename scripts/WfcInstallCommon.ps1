# Helpers shared by Install-Wfc.ps1 and Install-WfcLibrary.ps1; dot-sourced.
# Compatible with Windows PowerShell 5.1.

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Get-NativeArchitecture {
    $machine = $env:PROCESSOR_ARCHITEW6432
    if (-not $machine) { $machine = $env:PROCESSOR_ARCHITECTURE }
    switch ($machine) {
        'ARM64' { return 'arm64' }
        'AMD64' { return 'x64' }
        default { return 'x86' }
    }
}

function Get-LibraryVersion([string]$Path) {
    $info = (Get-Item -LiteralPath $Path).VersionInfo
    return "$($info.FileMajorPart).$($info.FileMinorPart)." +
        "$($info.FileBuildPart)"
}

# The libraries that register with Windows (they export DllRegisterServer).
$script:RegisteredLibraries = @(
    'wfcvbrun', 'wfcmscomctl', 'wfccomdlg32', 'wfcmswinsck'
)

# The regsvr32 that loads a library of the given architecture, or $null when
# this machine cannot run it.
function Get-Regsvr32([string]$Architecture) {
    $native = Get-NativeArchitecture
    if ($Architecture -eq 'x86' -and $native -ne 'x86') {
        return Join-Path $env:SystemRoot 'SysWOW64\regsvr32.exe'
    }
    if ($Architecture -eq $native) {
        return Join-Path $env:SystemRoot 'System32\regsvr32.exe'
    }
    return $null
}

# Registers (or unregisters) one library DLL with Windows. The scope picks the
# registry (Machine: HKLM, User: HKCU); MicrosoftIds also serves the
# Microsoft ProgIDs and class ids of the controls.
function Invoke-LibraryRegistration {
    param(
        [Parameter(Mandatory)][string]$Dll,
        [Parameter(Mandatory)][string]$Architecture,
        [Parameter(Mandatory)][string]$Scope,
        [switch]$MicrosoftIds,
        [switch]$Unregister
    )
    $regsvr32 = Get-Regsvr32 $Architecture
    if (-not $regsvr32) {
        Write-Warning ("Cannot register the $Architecture library " +
            "$(Split-Path -Leaf $Dll) on this machine.")
        return
    }
    $names = 'WFC_REGISTER_SCOPE', 'WFC_REGISTER_MICROSOFT_IDS'
    $saved = @{}
    foreach ($name in $names) {
        $saved[$name] = [Environment]::GetEnvironmentVariable($name)
    }
    try {
        $env:WFC_REGISTER_SCOPE = if ($Scope -eq 'Machine') {
            'machine'
        } else {
            'user'
        }
        $env:WFC_REGISTER_MICROSOFT_IDS = if ($MicrosoftIds) {
            '1'
        } else {
            '0'
        }
        $arguments = @('/s')
        if ($Unregister) { $arguments += '/u' }
        $arguments += "`"$Dll`""
        $process = Start-Process -FilePath $regsvr32 `
            -ArgumentList $arguments -Wait -PassThru -NoNewWindow
        if ($process.ExitCode -ne 0) {
            Write-Warning ("regsvr32 failed for $Dll (exit " +
                "$($process.ExitCode)).")
        }
    } finally {
        foreach ($name in $names) {
            [Environment]::SetEnvironmentVariable($name, $saved[$name])
        }
    }
}
