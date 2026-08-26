[CmdletBinding()]
param(
    [Parameter()]
    [string] $InputPath = 'C:\Windows\SysWOW64\msvbvm60.dll',

    [Parameter()]
    [string] $DumpbinPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x86\dumpbin.exe',

    [Parameter()]
    [string] $OutputPath = '.\evidence\reference\msvbvm60-6.0-runtime.json'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-U16 { param([byte[]] $Data, [int] $Offset) [BitConverter]::ToUInt16($Data, $Offset) }
function Get-U32 { param([byte[]] $Data, [int] $Offset) [BitConverter]::ToUInt32($Data, $Offset) }

function Get-AsciiZ {
    param([byte[]] $Data, [int] $Offset)
    $end = $Offset
    while ($end -lt $Data.Length -and $Data[$end] -ne 0) { $end++ }
    return [Text.Encoding]::ASCII.GetString($Data, $Offset, $end - $Offset)
}

function Convert-RvaToOffset {
    param([uint32] $Rva, $Sections)
    foreach ($section in $Sections) {
        $span = [Math]::Max([uint32] $section.virtualSize, [uint32] $section.rawSize)
        if ($Rva -ge $section.virtualAddress -and $Rva -lt ($section.virtualAddress + $span)) {
            return [int] ($section.rawOffset + ($Rva - $section.virtualAddress))
        }
    }
    throw ('RVA 0x{0:X8} is outside every PE section.' -f $Rva)
}

function Get-ExportCategory {
    param([AllowNull()][string] $Name)
    if ([string]::IsNullOrEmpty($Name)) { return 'ordinal-only' }
    if ($Name -match '^(BASIC_|EVENT_|Zombie_|Eb|Tip|Dll|VBDll)' -or
        $Name -match '^(ThunRTMain|UserDllMain|ProcCallEngine|MethCallEngine|CreateIExprSrvObj)$' -or
        $Name -match '^IID_') {
        return 'engine-and-hosting'
    }
    if ($Name -match '^(GetMem|PutMem|SetMem)' -or $Name -match '^(CopyRecord|VarPtr)$') {
        return 'member-and-memory'
    }
    if ($Name -match '^__vba') { return 'compiler-helper' }
    if ($Name -match '^rtc?' ) { return 'language-runtime' }
    if ($Name -match '^(_CI|_adj_|_allmul)') { return 'numeric-support' }
    return 'other'
}

$resolvedInput = (Resolve-Path -LiteralPath $InputPath).Path
$resolvedDumpbin = (Resolve-Path -LiteralPath $DumpbinPath).Path
$resolvedOutput = [IO.Path]::GetFullPath($OutputPath)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($resolvedOutput)) | Out-Null

$data = [IO.File]::ReadAllBytes($resolvedInput)
if ((Get-U16 $data 0) -ne 0x5A4D) { throw 'Input is not an MZ executable.' }
$peOffset = [int] (Get-U32 $data 0x3c)
if ((Get-U32 $data $peOffset) -ne 0x00004550) { throw 'Input has no PE signature.' }

$fileHeader = $peOffset + 4
$machine = Get-U16 $data $fileHeader
$sectionCount = Get-U16 $data ($fileHeader + 2)
$timeDateStamp = Get-U32 $data ($fileHeader + 4)
$optionalSize = Get-U16 $data ($fileHeader + 16)
$characteristics = Get-U16 $data ($fileHeader + 18)
$optional = $fileHeader + 20
$magic = Get-U16 $data $optional
if ($magic -ne 0x10b) { throw 'The reference runtime must be a PE32 image.' }

$sections = [System.Collections.Generic.List[object]]::new()
$sectionTable = $optional + $optionalSize
for ($index = 0; $index -lt $sectionCount; $index++) {
    $offset = $sectionTable + (40 * $index)
    $nameBytes = $data[$offset..($offset + 7)]
    $nameLength = [Array]::IndexOf($nameBytes, [byte] 0)
    if ($nameLength -lt 0) { $nameLength = 8 }
    $sections.Add([ordered]@{
        name = [Text.Encoding]::ASCII.GetString($nameBytes, 0, $nameLength)
        virtualSize = Get-U32 $data ($offset + 8)
        virtualAddress = Get-U32 $data ($offset + 12)
        rawSize = Get-U32 $data ($offset + 16)
        rawOffset = Get-U32 $data ($offset + 20)
        characteristics = Get-U32 $data ($offset + 36)
    })
}

$exportRva = Get-U32 $data ($optional + 96)
$exportSize = Get-U32 $data ($optional + 100)
$exportOffset = Convert-RvaToOffset $exportRva $sections
$ordinalBase = Get-U32 $data ($exportOffset + 16)
$functionCount = Get-U32 $data ($exportOffset + 20)
$nameCount = Get-U32 $data ($exportOffset + 24)
$functionTable = Convert-RvaToOffset (Get-U32 $data ($exportOffset + 28)) $sections
$nameTable = Convert-RvaToOffset (Get-U32 $data ($exportOffset + 32)) $sections
$ordinalTable = Convert-RvaToOffset (Get-U32 $data ($exportOffset + 36)) $sections

$namesByIndex = @{}
for ($index = 0; $index -lt $nameCount; $index++) {
    $nameRva = Get-U32 $data ($nameTable + (4 * $index))
    $nameOffset = Convert-RvaToOffset $nameRva $sections
    $ordinalIndex = Get-U16 $data ($ordinalTable + (2 * $index))
    $namesByIndex[[int] $ordinalIndex] = Get-AsciiZ $data $nameOffset
}

$exports = [System.Collections.Generic.List[object]]::new()
for ($index = 0; $index -lt $functionCount; $index++) {
    $rva = Get-U32 $data ($functionTable + (4 * $index))
    $name = if ($namesByIndex.ContainsKey($index)) { [string] $namesByIndex[$index] } else { $null }
    $forwarder = $null
    if ($rva -ge $exportRva -and $rva -lt ($exportRva + $exportSize)) {
        $forwarder = Get-AsciiZ $data (Convert-RvaToOffset $rva $sections)
    }
    $exports.Add([ordered]@{
        ordinal = [uint32] ($ordinalBase + $index)
        name = $name
        rva = ('0x{0:X8}' -f $rva)
        present = ($rva -ne 0)
        forwarder = $forwarder
        category = Get-ExportCategory $name
    })
}

$dependencyLines = @(& $resolvedDumpbin /dependents $resolvedInput)
$dependencies = @($dependencyLines | ForEach-Object {
    if ($_ -match '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') { $Matches[1] }
} | Select-Object -Unique)

$file = Get-Item -LiteralPath $resolvedInput
$hash = Get-FileHash -LiteralPath $resolvedInput -Algorithm SHA256
$signature = Get-AuthenticodeSignature -LiteralPath $resolvedInput
$categoryCounts = [ordered]@{}
foreach ($group in @($exports | Group-Object { $_['category'] } | Sort-Object Name)) {
    $categoryCounts[$group.Name] = $group.Count
}

$model = [ordered]@{
    schemaVersion = 1
    generatedUtc = [DateTime]::UtcNow.ToString('o')
    source = [ordered]@{
        path = $resolvedInput
        sha256 = $hash.Hash
        size = $file.Length
        fileVersion = $file.VersionInfo.FileVersion
        productVersion = $file.VersionInfo.ProductVersion
        originalFilename = $file.VersionInfo.OriginalFilename
        signatureStatus = [string] $signature.Status
        signerSubject = if ($null -ne $signature.SignerCertificate) { $signature.SignerCertificate.Subject } else { $null }
    }
    pe = [ordered]@{
        machine = ('0x{0:X4}' -f $machine)
        architecture = 'x86'
        timeDateStamp = ('0x{0:X8}' -f $timeDateStamp)
        characteristics = ('0x{0:X4}' -f $characteristics)
        imageBase = ('0x{0:X8}' -f (Get-U32 $data ($optional + 28)))
        subsystem = Get-U16 $data ($optional + 68)
        dllCharacteristics = ('0x{0:X4}' -f (Get-U16 $data ($optional + 70)))
        sections = $sections
        dependencies = $dependencies
    }
    exportDirectory = [ordered]@{
        ordinalBase = $ordinalBase
        functionSlots = $functionCount
        namedExports = $nameCount
        presentSlots = @($exports | Where-Object present).Count
        categoryCounts = $categoryCounts
        exports = $exports
    }
}

$json = $model | ConvertTo-Json -Depth 12
[IO.File]::WriteAllText($resolvedOutput, $json + [Environment]::NewLine,
    [Text.UTF8Encoding]::new($false))
Write-Output $resolvedOutput
