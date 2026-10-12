<#
.SYNOPSIS
Runs a WFC COM server in one process and a WFC client in another.

.DESCRIPTION
Starts `wfc <server project> --server`, runs `wfc <client program>` against
it, and checks the client output. The server's classes are registered under
HKCU for the duration of the test (the ProgID lookup needs them) and removed
afterwards. The test exits 77 (skipped) when the per-user registration cannot
be written.

.PARAMETER Wfc
Path of the wfc executable.

.PARAMETER Server
Path of the server project (.vbp).

.PARAMETER Client
Path of the client program (.bas).

.PARAMETER Expected
Text the client output must contain.

.EXAMPLE
pwsh -NoProfile -File scripts/Test-ComServer.ps1 -Wfc wfc.exe `
    -Server s.vbp -Client c.bas -Expected 'Remote peer'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $Wfc,

    [Parameter(Mandatory)]
    [string] $Server,

    [Parameter(Mandatory)]
    [string] $Client,

    [Parameter(Mandatory)]
    [string] $Expected
)

$ErrorActionPreference = 'Stop'
$registered = $false
$process = $null
try {
    & $Wfc $Server --register | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'cannot write the per-user COM registration'
        exit 77
    }
    $registered = $true
    $process = Start-Process -FilePath $Wfc -PassThru -WindowStyle Hidden `
        -ArgumentList @("`"$Server`"", '--server', '--server-timeout', '30000')
    $output = ''
    for ($attempt = 0; $attempt -lt 10; $attempt++) {
        Start-Sleep -Seconds 2
        $output = & $Wfc $Client 2>&1 | Out-String
        if ($output -like "*$Expected*") {
            break
        }
    }
    Write-Host $output
    if ($output -notlike "*$Expected*") {
        Write-Host "expected output not found: $Expected"
        exit 1
    }
    exit 0
}
finally {
    if ($null -ne $process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
    if ($registered) {
        & $Wfc $Server --unregister | Out-Null
    }
}
