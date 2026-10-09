# Retired Discovery Tools

**Content type:** Tool directory guide

**Status:** Retired (maintainer decision, 2026-10-09)

These PowerShell scripts performed the MP-0001 static discovery. They exported
the installed VB6, VBA, `stdole`, `MSComctlLib`, and `MSVBVM60.DLL` type
information into `evidence/reference/*.json` and generated the initial
requirement records (`REQ-0001`–`REQ-0131`) from it.

| Script | Produced |
| --- | --- |
| `Export-TypeLibrary.ps1`, `Export-MsComctlTypeLibrary.ps1` | Type-library evidence JSON |
| `Export-VbRuntime.ps1` | `msvbvm60-6.0-runtime.json` export inventory |
| `New-*Requirements.ps1` | Initial requirement records and their indexes |

They are kept for provenance only and must not be re-run against the
repository: the generated requirements have since been edited by hand (status
changes, dispositions, tailoring), and a re-run would overwrite those
decisions. Running them also needs the VB6 type-library tools and 32-bit
Windows PowerShell.

Because they are retired, they are outside the owned-source scope of the WSP
source-style and PowerShell profiles (`WSP-STYLE-0001`–`0007`,
`WSP-LANG-0007`); see the WSP adoption record. A future discovery tool is
written as new, in-scope source rather than by reviving these.
