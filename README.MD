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

The initial work plan was [MP-0001 — Foundation and
Feasibility](planning/MP-0001-foundation-and-feasibility.md), now complete.
The active milestone is MP-0002 — Core VB/VBA Language Execution, tracked in
[`planning/work-log-mp-0002.md`](planning/work-log-mp-0002.md) with delivered
scope indexed in [`requirements/language/README.md`](requirements/language/README.md);
see [`planning/README.md`](planning/README.md) for the full planning index.
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

`wfc --eval <source>` runs a standalone module; `wfc --class <Name> <source>`
(repeatable) supplies one or more class modules alongside it, standing in for
a real VB6 project's separate `.cls` files. The evaluator currently covers,
at a high level: declarations (`Dim`/`Const`/`Static`), every numeric type
(`Integer`, `Long`, `Single`, `Double`, `Currency`, `Decimal` via `Variant`)
plus `String`/`Boolean`/`Variant`, control flow (`If`/`Select Case`/`While`/
`Do`/`For`/`For Each`), fixed-size (including multi-dimensional) and
dynamic (`ReDim`/`ReDim Preserve`/`Erase`) arrays, including
`Variant`/`Object`-element arrays, the core `Strings`/
`Conversion`/`Information`/`Math` intrinsic library, user-defined `Sub`/
`Function` procedures (`Optional`/`ParamArray`/`Static`/array parameters,
and a zero-argument call without parentheses), and a minimal
class-module foundation (fields, methods, `Property Get`/`Let`/`Set`,
`New`/`Me`, lifecycle hooks, `Public`/`Private`). It does not claim general
VB6 compatibility — see the [language requirements
index](requirements/language/README.md) for exactly what is specified and
implemented, `REQ`-numbered and cross-referenced to its diagnostics and
scope boundary, and [`planning/work-log-mp-0002.md`](planning/work-log-mp-0002.md)
for the chronological build-out.

A representative sample (the exhaustive, always-current set of runnable
examples is the `TC-MP0002-*-cli` integration tests in `CMakeLists.txt`):

```powershell
# Basics and control flow
out/build/windows-x64-debug/Debug/wfc.exe --eval 'Print "Hello, WFC!"'
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim total As Long`nIf True Then`n total = 42`nEnd If`nPrint total"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim i As Long`nFor i = 1 To 10`n If i = 3 Then`n  Exit For`n End If`nNext i`nPrint i"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Select Case 7`nCase Is >= 5`n Print `"selected`"`nEnd Select"

# Types, conversions, and strings
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim x As Currency`nx = 10.5`nPrint x & `" `" & TypeName(x) & `" `" & (10@ / 3@)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim x`nx = 5`nPrint TypeName(x)`nx = Null`nPrint CStr(IsNull(x))"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Print Replace(InStrRev(`"one two one`", `"one`") & `": `" & Mid$(`"Hello, WFC!`", 8, 3), `"WFC`", `"world`")"

# Arrays (fixed-size and dynamic) and the minimal object stub
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim arr(3) As Long`narr(0) = 10`narr(3) = 40`nPrint arr(0) & `" `" & arr(3) & `" `" & LBound(arr) & `" `" & UBound(arr)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim arr() As Long`nReDim arr(3)`narr(0) = 1`narr(3) = 4`nReDim Preserve arr(5)`nPrint arr(0) & `" `" & arr(3) & `" `" & UBound(arr)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim arr(3) As Long`nDim x As Long`nDim total As Long`narr(0)=10`narr(1)=20`narr(2)=30`narr(3)=40`nFor Each x In arr`ntotal = total + x`nNext x`nErase arr`nPrint total & `" `" & UBound(arr)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim grid(1 To 2, 1 To 3) As Long`ngrid(1, 1) = 1`ngrid(2, 3) = 6`nPrint grid(1, 1) & `" `" & grid(2, 3) & `" `" & LBound(grid, 2) & `" `" & UBound(grid, 2)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim arr(2) As Variant`narr(0) = 5`narr(1) = `"hello`"`narr(2) = True`nPrint TypeName(arr(0)) & `" `" & TypeName(arr(1)) & `" `" & TypeName(arr(2)) & `" `" & TypeName(arr)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Dim x As Object`nPrint TypeName(Nothing)`nSet x = Nothing`nPrint CStr(x Is Nothing)"

# Procedures, with Optional/ParamArray/array parameters/parenless calls
out/build/windows-x64-debug/Debug/wfc.exe --eval "Function Factorial(n As Long) As Long`nIf n <= 1 Then`nFactorial = 1`nElse`nFactorial = n * Factorial(n - 1)`nEnd If`nEnd Function`nPrint Factorial(5)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Function Greet(name As String, Optional greeting As String = `"Hello`") As String`nGreet = greeting & `", `" & name`nEnd Function`nFunction Total(ParamArray nums() As Long) As Long`nDim i As Long`nDim s As Long`nFor i = LBound(nums) To UBound(nums)`ns = s + nums(i)`nNext i`nTotal = s`nEnd Function`nPrint Greet(`"World`") & `" | `" & Total(1, 2, 3, 4)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Sub DoubleAll(nums() As Long)`nDim i As Long`nFor i = LBound(nums) To UBound(nums)`nnums(i) = nums(i) * 2`nNext i`nEnd Sub`nDim arr(2) As Long`narr(0) = 1`narr(1) = 2`narr(2) = 3`nCall DoubleAll(arr)`nPrint arr(0) & `" `" & arr(1) & `" `" & arr(2)"
out/build/windows-x64-debug/Debug/wfc.exe --eval "Function NextId() As Long`nStatic counter As Long`ncounter = counter + 1`nNextId = counter`nEnd Function`nPrint NextId`nCall NextId`nPrint NextId & `" `" & Rnd"

# Class modules: fields, methods, properties, New/Me, lifecycle, Private
out/build/windows-x64-debug/Debug/wfc.exe --class Counter "Public value As Long`n`nSub Increment()`nvalue = value + 1`nEnd Sub`n`nProperty Get Doubled() As Long`nDoubled = value * 2`nEnd Property" --eval "Dim c As New Counter`nCall c.Increment()`nCall c.Increment()`nPrint c.value & `" `" & c.Doubled & `" `" & TypeName(c)"
out/build/windows-x64-debug/Debug/wfc.exe --class Foo "Private n As Long`n`nSub Class_Initialize()`nn = 100`nEnd Sub`n`nSub Class_Terminate()`nPrint `"term `" & n`nEnd Sub`n`nSub Bump()`nn = n + 1`nPrint Me.n`nEnd Sub" --eval "Dim x As New Foo`nCall x.Bump()`nSet x = Nothing"
out/build/windows-x64-debug/Debug/wfc.exe --class Pair "Public a As Long`nPublic b As Long`n`nProperty Get Item(i As Long) As Long`nIf i = 0 Then`nItem = a`nElse`nItem = b`nEnd If`nEnd Property`n`nProperty Let Item(i As Long, v As Long)`nIf i = 0 Then`na = v`nElse`nb = v`nEnd If`nEnd Property" --eval "Dim p As New Pair`np.Item(0) = 10`np.Item(1) = 20`nPrint p.Item(0) & `" `" & p.Item(1)"
```
