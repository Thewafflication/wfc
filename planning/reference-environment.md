# Reference Environment — VB6 Discovery Baseline

**Content type:** Project planning record  
**Status:** Active  
**Recorded:** 2026-08-26

This record identifies the reference environment used to inspect and characterize
the VB6 ecosystem during MP-0001. It is not a statement that every discovered
behavior will be duplicated; compatibility decisions require a project
requirement and verification evidence.

## Reference Product Components

| Component | Path | Version |
| --- | --- | --- |
| Visual Basic 6 IDE | `C:\Program Files (x86)\Microsoft Visual Studio\VB98\VB6.EXE` | 6.00.8176 |
| Visual Basic object library | `C:\Program Files (x86)\Microsoft Visual Studio\VB98\VB6.OLB` | Type library 6.0 |
| Visual Basic for Applications library | `C:\Program Files (x86)\Microsoft Visual Studio\VB98\VBA6.DLL` | 6.0.8169 |
| Visual Basic runtime | `C:\Windows\SysWOW64\msvbvm60.dll` | 6.00.9848 |
| Standard OLE type library | `C:\Windows\SysWOW64\stdole2.tlb` | Type library 2.0 |
| Microsoft Windows Common Controls 6.0 | `C:\Windows\SysWOW64\MSCOMCTL.OCX` | 6.00.8177 |

The reference components are x86. Tests and tools that load them shall run in
an x86 process; x64 tooling may inspect files but cannot load these in-process.

## Available Discovery Tools

| Tool | Path | Use |
| --- | --- | --- |
| OLE/COM Object Viewer | `C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\oleview.exe` | Extract COM registrations and type-library contracts as reconstructed IDL |
| Dumpbin | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x86\dumpbin.exe` | Inspect PE headers, imports, exports, and dependent DLL names |
| Process Monitor | `C:\ProgramData\chocolatey\bin\Procmon.exe` | Record file-system, Registry, process, thread, and DLL activity during probes |
| Inspect | `C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\inspect.exe` | Record UI Automation and accessibility information for visible controls |

## Collection Procedure

For each target library, runtime feature, or control:

1. Record the binary identity: path, SHA-256, file/product version,
   architecture, COM registration, and type-library GUID/version.
2. Save the exported type-library/IDL view and derive a member inventory.
3. Write the smallest VB6 probe that exercises one specified behavior.
4. Capture its source, expected observable result, and actual result. Add a
   filtered Procmon trace or Inspect capture when behavior depends on system or
   UI interaction.
5. Create a project requirement that states the intended compatibility claim,
   then allocate an automated WFC test to it.

Store probes and retained evidence under source control once the project
testing structure is established. Do not commit proprietary Microsoft binaries
or distribute extracted material beyond what is permitted by their licenses.
