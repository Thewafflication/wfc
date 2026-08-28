# WFC — Waughtal Foundation Classes

A C++ reimplementation of the VB6 ecosystem:

* VB
* VBA
* VBRUN
* stdole
* MSComctlLib

The project targets Windows x86, x64, and ARM64 and uses WCRT and WSP as its
runtime and engineering-process foundations.

## Planning

The [1.0 compatibility profile](planning/compatibility-profile-1.0.md) defines
the mostly complete replacement claim and its explicit exclusions. The
[roadmap to 1.0.0](planning/roadmap-1.0.md) divides delivery into evidence-gated
milestones from `0.1.0` through release.

The initial work plan is [MP-0001 — Foundation and Feasibility](planning/MP-0001-foundation-and-feasibility.md).
The [VB6 discovery baseline](planning/reference-environment.md) records the
installed reference components and inspection tools.

The [project requirements index](requirements/README.md) contains the proposed
compatibility contracts produced by discovery.

The [architecture overview](architecture/README.md) defines the component and
dependency boundaries for the implementation.

The [Visual Basic class requirements](requirements/vb/README.md) identify the
form hosts, intrinsic controls, run-time service classes, and behavioral
contracts exposed by VB6.

The [VBA requirements](requirements/vba/README.md) inventory the standard
library classes, function modules, enumerations, and constants.

The [standard OLE requirements](requirements/stdole/README.md) capture the
font, picture, persistence-function, and constant contracts used by VB6 forms.

The [VB6 runtime requirements](requirements/vbrun/README.md) capture the native
runtime binary, export layout, compiler helpers, and language services.

## Build the Current Development Slice

Prerequisites are Visual Studio 2022 with the Desktop development with C++
workload and CMake 3.28 or newer. Initialize the controlled dependencies after
cloning:

```powershell
git submodule update --init --recursive
```

Configure, build, and test the native x64 debug slice:

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
```

Equivalent `windows-x86-debug` and `windows-arm64-debug` presets are provided.
ARM64 execution evidence must be collected on a native or approved emulated
ARM64 runner; a successful cross-build alone is not an execution result.

The current command accepts sequential `Dim`, assignment, and `Print`
statements separated by line endings or `:`. Variables may be `Long`, `String`,
or `Boolean`; expressions support literals, variables, parentheses, arithmetic,
concatenation, same-type comparisons, and strict Boolean `Not`, `And`, `Or`,
`Xor`, `Eqv`, and `Imp`. It remains an early MP-0002 subset and does not claim
general VB6 compatibility:

```powershell
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Print "Hello, WFC!"'
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Print (2 + 3) * 4 & " items"'
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Dim total As Long: total = 6 * 7: Print total'
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Dim ready As Boolean: ready = 6 * 7 = 42: Print ready And Not False'
```
