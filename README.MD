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

The current command accepts sequential `Dim`, assignment, `Print`, and
single-line `If <Boolean expression> Then <branch> [Else <branch>]` and block
`If <Boolean expression> Then` / ordered `ElseIf` / optional `Else` / `End If`
statements, plus `While <Boolean expression>` / `Wend` loops. Pre-test and
post-test `Do While` / `Do Until` loops terminate with `Loop`; `Exit Do`
terminates the innermost active `Do` loop. Declared-`Long` `For...Next` loops
support inclusive bounds and an optional `Step`; `Exit For` terminates the
innermost active counter loop. `Select Case` supports ordered, same-type
equality branches, comma-separated values, inclusive ranges, `Case Is`
relations, and `Case Else`.
A single-line conditional branch is either `Print <expression>` or an
assignment. Variables may be `Long`, `String`, or `Boolean`; expressions
support literals, variables, typed constants, parentheses, arithmetic,
concatenation, same-type comparisons, and strict Boolean `Not`, `And`, `Or`,
`Xor`, `Eqv`, and `Imp`. It remains an early MP-0002 subset and does not claim
general VB6 compatibility:

```powershell
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Print "Hello, WFC!"'
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Print (2 + 3) * 4 & " items"'
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Dim total As Long: total = 6 * 7: Print total'
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Dim ready As Boolean: ready = 6 * 7 = 42: Print ready And Not False'
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Dim total As Long: If True Then total = 42 Else total = 0: If total = 42 Then Print "selected"'
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim total As Long`nIf True Then`n total = 42`nEnd If`nPrint total"
out/build/windows-x64-debug/Debug/wfc.exe --eval "If False Then`n Print `"first`"`nElseIf True Then`n Print `"selected`"`nEnd If"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim index As Long`nWhile index < 3`n Print index`n index = index + 1`nWend"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim index As Long`nDo Until index = 3`n Print index`n index = index + 1`nLoop"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim index As Long`nDo`n Print index`n index = index + 1`nLoop While index < 3"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Rem a VB comment`nPrint 42"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Do While True`n Print `"once`"`n Exit Do`nLoop"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim i As Long`nFor i = 1 To 3`n Print i`nNext i"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim i As Long`nFor i = 1 To 10`n If i = 3 Then`n  Exit For`n End If`nNext i`nPrint i"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Select Case 2`nCase 1`n Print `"one`"`nCase 2`n Print `"two`"`nEnd Select"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Select Case 2`nCase 1, 2, 3`n Print `"selected`"`nEnd Select"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Select Case 7`nCase Is >= 5`n Print `"selected`"`nEnd Select"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Const answer As Long = 6 * 7`nPrint answer"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Option Explicit`nDim value As Long`nvalue = 42`nPrint value"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Option Compare Text`nPrint `"Alpha`" = `"alpha`""
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Len(`"Hello, WFC!`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print LCase$(`"Hello, WFC!`")`nPrint UCase(`"Hello, WFC!`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print `"[`" & Trim$(`"  Hello, WFC!  `") & `"]`""
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Left$(`"Hello`", 2) & Right$(`"WFC!`", 2)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Mid$(`"Hello, WFC!`", 8, 3)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Chr$(Asc(`"W`")) & Chr$(70) & Chr$(67)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print StrReverse(`"!CFW ,olleH`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print `"[`" & Space$(3) & `"]`""
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print String$(3, `"*`") & String(2, 61)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print InStr(`"Hello, WFC!`", `"WFC`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print InStr(3, `"abcabc`", `"bc`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print StrComp(`"abc`", `"abd`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Replace(`"abcabc`", `"b`", `"X`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print `"&H`" & Hex(255) & `" `" & Oct(8)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print `"[`" & Str(42) & `"]`""
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print InStr(1, `"aXbXc`", `"x`", vbTextCompare)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Replace(`"prefix aXbXc`", `"x`", `"-`", 8, 1, vbTextCompare)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print InStrRev(`"one two one`", `"one`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Val(`" 2 45 7th Street`")"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Abs(-42) & `" `" & Sgn(-42)"
```
