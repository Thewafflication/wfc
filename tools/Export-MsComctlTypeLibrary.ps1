[CmdletBinding()]
param(
    [Parameter()]
    [string] $InputPath = 'C:\Windows\SysWOW64\MSCOMCTL.OCX',

    [Parameter(Mandatory)]
    [string] $OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ([Environment]::Is64BitProcess) {
    throw 'Run this script with 32-bit Windows PowerShell because TLBINF32 is an x86 COM server.'
}

function Get-TypeName {
    param([Parameter(Mandatory)] $Type)

    $typeName = $null
    try {
        if ($null -ne $Type.TypeInfo) {
            $typeName = [string] $Type.TypeInfo.Name
        }
    }
    catch {
        $typeName = $null
    }

    if ([string]::IsNullOrEmpty($typeName)) {
        $varTypes = @{
            0 = 'Void'; 2 = 'Integer'; 3 = 'Long'; 4 = 'Single'; 5 = 'Double'
            6 = 'Currency'; 7 = 'Date'; 8 = 'String'; 9 = 'Object'
            11 = 'Boolean'; 12 = 'Variant'; 13 = 'IUnknown'; 16 = 'Byte'
            17 = 'Byte'; 18 = 'UInteger'; 19 = 'ULong'; 20 = 'LongLong'
            21 = 'ULongLong'; 22 = 'Integer'; 23 = 'UInteger'; 24 = 'Void'
            25 = 'HRESULT'; 26 = 'Pointer'; 27 = 'SafeArray'; 28 = 'CArray'
            29 = 'UserDefined'; 30 = 'LPStr'; 31 = 'LPWStr'; 36 = 'Record'
        }
        $varType = [int] $Type.VarType
        $baseType = $varType -band 0x0fff
        if ($varTypes.ContainsKey($baseType)) {
            $typeName = $varTypes[$baseType]
        }
        else {
            $typeName = 'VarType(' + $varType + ')'
        }
        if (($varType -band 0x2000) -ne 0) {
            $typeName += '()'
        }
        if (($varType -band 0x4000) -ne 0) {
            $typeName = 'ByRef ' + $typeName
        }
    }

    return $typeName
}

function Get-MemberKind {
    param([int] $InvokeKind)

    switch ($InvokeKind) {
        1 { 'Method' }
        2 { 'PropertyGet' }
        4 { 'PropertyLet' }
        8 { 'PropertySet' }
        default { 'InvokeKind(' + $InvokeKind + ')' }
    }
}

function Get-InterfaceModel {
    param([Parameter(Mandatory)] $Interface)

    $members = [System.Collections.ArrayList]::new()
    for ($memberIndex = 1; $memberIndex -le $Interface.Members.Count; $memberIndex++) {
        $member = $Interface.Members.Item($memberIndex)
        if (([int] $member.AttributeMask -band 1) -ne 0) {
            continue
        }

        $parameters = [System.Collections.ArrayList]::new()
        for ($parameterIndex = 1; $parameterIndex -le $member.Parameters.Count; $parameterIndex++) {
            $parameter = $member.Parameters.Item($parameterIndex)
            $parameterFlags = [int] $parameter.Flags
            $isOptional = [bool] $parameter.Optional -or (($parameterFlags -band 16) -ne 0)
            $hasDefault = [bool] $parameter.Default -or (($parameterFlags -band 32) -ne 0)
            $defaultValue = $null
            if ($hasDefault) {
                try { $defaultValue = $parameter.DefaultValue } catch { $defaultValue = $null }
            }
            [void] $parameters.Add([ordered]@{
                name = [string] $parameter.Name
                type = Get-TypeName -Type $parameter.VarTypeInfo
                flags = $parameterFlags
                optional = $isOptional
                hasDefault = $hasDefault
                defaultValue = $defaultValue
            })
        }

        [void] $members.Add([ordered]@{
            name = [string] $member.Name
            kind = Get-MemberKind -InvokeKind ([int] $member.InvokeKind)
            memberId = [int] $member.MemberId
            flags = [int] $member.AttributeMask
            returnType = Get-TypeName -Type $member.ReturnType
            parameters = $parameters
        })
    }

    return [ordered]@{
        name = [string] $Interface.Name
        guid = [string] $Interface.GUID
        flags = [int] $Interface.AttributeMask
        members = $members
    }
}

$resolvedInput = (Resolve-Path -LiteralPath $InputPath).Path
$resolvedOutput = [IO.Path]::GetFullPath($OutputPath)
$outputDirectory = [IO.Path]::GetDirectoryName($resolvedOutput)
if (-not [string]::IsNullOrEmpty($outputDirectory)) {
    [IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
}

$application = New-Object -ComObject TLI.TLIApplication
$library = $application.TypeLibInfoFromFile($resolvedInput)
$controls = [System.Collections.ArrayList]::new()

for ($classIndex = 1; $classIndex -le $library.CoClasses.Count; $classIndex++) {
    $class = $library.CoClasses.Item($classIndex)
    if (([int] $class.AttributeMask -band 0x20) -eq 0) {
        continue
    }

    $progId = $null
    try {
        $progIdKey = 'Registry::HKEY_CLASSES_ROOT\CLSID\' + [string] $class.GUID + '\ProgID'
        $progId = [string] (Get-ItemProperty -LiteralPath $progIdKey).'(default)'
    }
    catch {
        $progId = $null
    }

    $events = $null
    try {
        if ($null -ne $class.DefaultEventInterface) {
            $events = Get-InterfaceModel -Interface $class.DefaultEventInterface
        }
    }
    catch {
        $events = $null
    }

    [void] $controls.Add([ordered]@{
        name = [string] $class.Name
        guid = [string] $class.GUID
        progId = $progId
        flags = [int] $class.AttributeMask
        defaultInterface = Get-InterfaceModel -Interface $class.DefaultInterface
        defaultEventInterface = $events
    })
}

$file = Get-Item -LiteralPath $resolvedInput
$hash = Get-FileHash -LiteralPath $resolvedInput -Algorithm SHA256
$model = [ordered]@{
    schemaVersion = 1
    generatedUtc = [DateTime]::UtcNow.ToString('o')
    source = [ordered]@{
        path = $resolvedInput
        sha256 = $hash.Hash
        fileVersion = $file.VersionInfo.FileVersion
        productVersion = $file.VersionInfo.ProductVersion
    }
    library = [ordered]@{
        name = [string] $library.Name
        guid = [string] $library.GUID
        majorVersion = [int] $library.MajorVersion
        minorVersion = [int] $library.MinorVersion
        lcid = [int] $library.LCID
        sysKind = [int] $library.SysKind
        typeInfoCount = [int] $library.TypeInfoCount
    }
    controls = $controls
}

$json = $model | ConvertTo-Json -Depth 20
[IO.File]::WriteAllText($resolvedOutput, $json + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
Write-Output $resolvedOutput
