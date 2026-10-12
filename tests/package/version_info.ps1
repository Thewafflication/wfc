# Prints the version resource fields of an executable, separated by `|`.
param([Parameter(Mandatory)][string]$Path)
$v = (Get-Item -LiteralPath $Path).VersionInfo
$fields = $v.CompanyName, $v.ProductName, $v.FileDescription,
    $v.FileVersion, $v.LegalCopyright, $v.OriginalFilename
$fields -join '|'
