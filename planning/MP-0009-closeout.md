# MP-0009 — Native Compilation: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0009 — Native compilation (`0.9.0`)

**Status:** Drafted, awaiting the owner's decision on the deferrals below

**Completion date:** 2026-10-28 (construction); the closeout decision is the owner's

**Inherited baseline:** MP-0008 closeout (`MP-0008-closeout.md`), release `0.8.0`

**Tested baseline:** windows-x64-debug, 493 of 493 CTest tests passing on the
closing commit; the GitHub Actions matrix (Debug and Release on x86, x64 and
ARM64, and lint) is green on the last five completed runs

**Owner:** Project maintainer

**Approval:** Pending. The plan itself was drafted at the MP-0008 closeout and
its formal approval was never recorded.

## Outcome

WFC compiles VB6 projects to machine code for x86, AMD64 and ARM64. Each
architecture has its own back end (`src/compiler/x64.cpp`, `x86.cpp`,
`arm64.cpp`) behind one intermediate representation and one lowering pass
(`lower.cpp`); generated code reaches the shared C++ runtime only through a
versioned entry table (`ir::Service`, `kRuntimeApiVersion`). `wfc --native`
compiles and runs a program or project; `wfc --build ... --native` packages the
machine code with the design of the forms, the resources and the files of the
project, and the packaged program loads that code.

- **Language coverage.** Procedures, all the control flow, `GoTo`, line numbers
  and `Erl`, `On Error` and `Resume`, `Const`, `Select Case`, `Static`,
  `Option Base`, `Option Compare Text`, conditional compilation, `Def`
  statements, strings (UTF-8 in memory with UTF-16 results), integers, floating
  point, `Date`, `Currency`, `Decimal` and Variants, arrays of one to three
  dimensions (fixed and dynamic, of any element type, also inside records and
  classes), user-defined types (nested, with arrays, Variants and fixed-length
  Strings), classes with properties, events, `Implements`, default members,
  `Static` per instance, global name spaces and `NewEnum`, late binding and
  COM objects (type-library classes and constants, `WithEvents`), forms and
  controls, files, `Declare`d DLL routines, and the VBA library.
- **Corpus.** All 208 programs of `tests/corpus` compile natively and print and
  end the same as the interpreter (`work-log-mp-0009.md`, row 39). The seven
  games of MP-0008 play natively (`TC-MP0009-native-vb6games-*`) and are
  packaged natively (`TC-MP0009-native-package-*`). 73 programs and projects
  in `tests/native` are compared with the interpreter by CTest on every
  architecture (`TC-MP0009-native-*`).
- **Speed** (Release, x64, one run, startup included):

  | Program | Interpreter | Native |
  | --- | --- | --- |
  | `44-algorithms` (sorting, primes) | 0.61 s | 0.03 s |
  | `01-fizzbuzz` | 0.28 s | 0.03 s |
  | `49-deep-recursion` | 0.17 s | 0.03 s |
  | `97-csv-bigint-bfs` | 0.14 s | 0.05 s |
  | Collatz search 1 to 30,000 (work log row 3) | 20.8 s | 0.07 s |

  Programs that spend their time in the shared runtime (COM, files, process
  start) take about as long as before, as expected.

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| ADRs: IR, calling convention, layouts, runtime entry table | Complete | `ADR-0008`, `ADR-0009` |
| IR and lowering | Complete | Native tests, work log |
| AMD64, x86 and ARM64 back ends | Complete; x86 and ARM64 are validated by CI only | CI matrix |
| Startup, module initialization, error flow | Complete (`On Error`, `Resume`, `End`, pending-error flag) | Tests 8 and following |
| The MP-0008 corpus built natively | Complete | `TC-MP0009-native-vb6games-*`, `TC-MP0009-native-package-*` |
| Packaging of native outputs | Complete for `--build --native` (the interpreter stays the default of `--build`) | `TC-MP0009-native-package-*` |
| Execution-speed measurements | Complete (table above) | This record |
| `REQ-0100`–`REQ-0104` helper behavior and signatures, `REQ-0105` exports bound to the helpers | **Not done** (see Deferred) | `wfcvbrun.dll` exports stay stubs |
| Legacy x86 binary corpus resolves its runtime imports on x86 | Not started: needs an approved corpus | Roadmap gate |

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| Native applications pass the compiler/runtime corpus on x86, x64 and ARM64 | CI: `TC-MP0009-native-*` on all six jobs | Required | Pass |
| The MP-0008 project corpus builds natively | `TC-MP0009-native-vb6games-*`, `TC-MP0009-native-package-*` | Required | Pass |
| Helper signatures and behavior of `REQ-0100`–`REQ-0104` | Tests | Required | **Not met** (deferral proposed below) |
| The approved legacy x86 binary corpus resolves its runtime imports on x86 | Tests | Roadmap gate | Not started |

## Variances

| Variance from the interpreter or the plan | Reason | Disposition |
| --- | --- | --- |
| A native program still contains the shared runtime, which includes the interpreter as the engine for Variants, COM, forms and the VBA library | `ADR-0009`: those parts are written once and shared | Accepted design; the program's own code is machine code |
| `Debug.Print` writes nothing in a native program (the interpreter writes it to standard error) | A compiled VB6 program has no Immediate window | Accepted |
| The interpreter prints an empty line on standard output at the end of some runs that only use `Debug.Print` | Interpreter artifact | Compared as equal |
| Interpreter defect: `GoTo` to a line that holds `Next` ends the `For` loop after the first pass; native code runs the loop as VB6 does | Found while writing test 61 | Not changed; to be filed |
| Five interpreter defects are filed as issues 1 to 5 (Static of a record, `Step 0`, `String(n, code)`, Variant ByRef write-back, screen metrics in headless runs) | Found while building the native tests | Open; the owner decides when |
| Strings are UTF-8 in memory; non-ASCII text goes through the interpreter's UTF-16 functions | Speed of the ASCII case | Accepted |
| `wfc --build` embeds the interpreter unless `--native` is given | The interpreter is the reference and the fallback | Open: decide the default at 1.0 |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| Behavior of the `REQ-0100`–`REQ-0105` helpers and exports of `wfcvbrun.dll` (the `MSVBVM60` runtime entry points) | The DLL exports names and ordinals only; no program calls the helpers | Project maintainer | Needs probes of a VB6 reference machine for the signatures | The native compiler does not use the DLL; the shared runtime implements the behavior | Pending |
| Approved legacy x86 binary corpus | The roadmap gate stays open | Project maintainer | Before 1.0 | None | Pending |
| Behavioral probes against a VB6 reference machine | Behavior rests on the interpreter's tests | Project maintainer | Before 1.0 | Interpreter tests | Pending (carried from MP-0008) |
| Items carried from MP-0008 (`MSCOMCTL` persistence, ImageList `Draw`, ListView hover, printer dialog, `AddressOf` callbacks, floating-point `Declare` arguments) | Some programs differ | Project maintainer | MP-0010 | Tests of the supported subset | Pending |

## Closeout Decision

- **Decision:** Not yet made. Proposed: complete with the deferrals above, once
  the owner accepts the deferral of `REQ-0100`–`REQ-0105`.
- **Approver:** Project maintainer
- **Rationale:** Every code objective of the plan is met and verified by CI on
  all three architectures; the one requirement group that is not met needs
  information (signatures from a VB6 reference machine) that does not exist in
  this project.
- **Next review or release action:** the owner's decision; then the `0.9.0`
  release (tag and signed packages) and `MP-0010`.

## References

- [MP-0009 plan](MP-0009-native-compilation.md)
- [Work log](work-log-mp-0009.md)
- [ADR-0008](../architecture/adr-0008-native-compilation-pipeline.md)
- [ADR-0009](../architecture/adr-0009-variants-objects-and-forms-in-native-programs.md)
