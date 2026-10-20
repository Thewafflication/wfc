# MP-0009 — Native Compilation

**Content type:** Milestone work plan  
**Status:** Planned  
**Target baseline:** 0.9.0  
**Planned period:** 2026-10-20 through milestone closeout  
**Inherited baseline:** MP-0008 closeout (`MP-0008-closeout.md`), `master` at the `0.8.0` release  
**Owner:** Project maintainer  
**Approval:** Plan drafted at the MP-0008 closeout, 2026-10-20; formal approval pending

## Objective and Scope

WFC generates native machine code for x86, AMD64 and ARM64 from VB6 projects.
Each architecture has its own back end sharing one runtime. There is no
translation layer, emulation or x86 helper process on x64 or ARM64 (owner
decision, 2026-10-12; `ADR-0007`). A built program no longer embeds the
interpreter.

In scope: an intermediate representation (IR); the three back ends; calling
conventions, startup, module initialization, stack and error flow, object and
array layouts; the compiler, member, language, numeric and COM helpers of
`REQ-0100` through `REQ-0104` and the `MSVBVM60` export behavior of
`wfcvbrun.dll`; execution speed; packaging of native outputs.

Out of scope: new language or control features (those belong to MP-0010), and
running legacy x86 `MSVBVM60.DLL` binaries on x64 or ARM64.

### Inherited (do not redo)

The interpreter and its behavior tests (MP-0002 to MP-0008), the corpus of
seven games, the `--build` interface and the packaged-program tests, the
`wfcvbrun.dll` export directory, and the Release CI matrix and signed WPM
packages (MP-0008).

## Approach

- **The interpreter is the reference.** The same corpus programs
  (`tests/corpus`, `tests/vb6games`) must print and show the same thing when
  built natively. Every native test compares native output with interpreter
  output.
- **Vertical slices.** One architecture first (AMD64, the machine in use): a
  procedure with integer arithmetic, then control flow, strings, arrays,
  classes, forms and controls, then the games; the x86 and ARM64 back ends
  reuse the IR and the runtime.
- **Runtime shared, not duplicated.** Strings, Variants, arrays, objects and
  the form and control host stay in the C++ runtime; generated code calls them
  through a documented, versioned entry table.
- **Decisions in ADRs first.** IR, calling convention and layout decisions are
  recorded before code that depends on them.

## Baseline and Assumptions

- The `0.8.0` release is the baseline; nothing in this plan changes it.
- No VB6 reference machine exists; behavior comes from the interpreter's tests.
- x86 and ARM64 code is validated by GitHub Actions only (`AGENTS.md`).

## Deliverables

| Deliverable | Completion evidence |
| --- | --- |
| ADRs: IR, calling convention, layouts, runtime entry table | `architecture/adr-*.md` |
| IR and lowering from the front end | Unit tests, IR dumps in tests |
| AMD64, x86 and ARM64 back ends | Native corpus tests per architecture |
| Startup, module initialization, error and unwind flow | Tests (`On Error`, `Resume`, `End`) |
| Helper behavior and signatures (`REQ-0100`–`REQ-0104`) | Requirement records, tests |
| The MP-0008 corpus built natively | Packaged-program tests, screenshots |
| Execution-speed measurements | Performance matrix record |
| Closeout record | `MP-0009-closeout.md` |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Planning | ADRs for IR, calling convention, layouts | Documents baselined |
| 2. IR | Lowering of the language subset the corpus uses | IR tests |
| 3. AMD64 slice | Procedures, arithmetic, control flow built and run | Native vs interpreter output |
| 4. Runtime | Strings, Variants, arrays, objects, errors | Corpus programs built natively |
| 5. Forms and controls | Native programs drive the same host | Games start and play |
| 6. x86 and ARM64 back ends | Same tests on all three | Green matrix |
| 7. Packaging | Native outputs replace the embedded interpreter | Package tests |
| 8. Closeout | Deferrals recorded | Required gates pass |

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| `REQ-0100`–`REQ-0104` | Runtime helpers behind the entry table | Signature and behavior tests | Yes |
| `REQ-0105` | `wfcvbrun.dll` exports bound to the helpers | Existing layout test plus helper calls | Yes |
| `REQ-0303` | Native `--build` output | Package tests | Yes |
| Corpus projects build natively | All stages | Acceptance tests per game | Yes |
| Speed | Back ends, runtime | Performance matrix | Reported |

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, decisions, completion evidence |
| Reviewer | Documented approved exception (single-maintainer project) | Review ADRs and requirement records |
| Verifier | Project maintainer | Execute and retain build/test evidence |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| Three back ends and no machine to run two of them locally | Late defects | Shared IR and runtime; every test runs on all three in CI | Stage 6 |
| VB6 semantics (Variants, late binding, `On Error`) are hard to compile | Divergence from the interpreter | Compare native with interpreter output on the corpus | Stage 4 |
| No fallback to the interpreter (no bridge, no translation layer) | A program that cannot compile does not run | Language coverage first, diagnostics for what is not compiled | Stage 2 |
| Size of the work | Delay | One slice at a time; replan after the AMD64 slice | Stage 3 |

## Estimate and Forecast

Forecast 40–80 increments, dominated by the runtime and the second and third
back ends once the first works. Replan after the AMD64 slice.

## Execution and Evidence

- Local x64 debug configure, build and CTest per `AGENTS.md`; x86 and ARM64 are
  validated by GitHub Actions.
- Progress is recorded in `work-log-mp-0009.md` (created with the first
  increment).

## Rollback and Recovery

The interpreter stays the default execution path until native output passes
the corpus; a native back end is switched on per architecture by a build
option, and reverting an increment is a normal Git change.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| Native applications pass the compiler/runtime corpus on x86, x64 and ARM64 | CI | Required | Not started |
| The MP-0008 project corpus builds natively | Acceptance tests | Required | Not started |
| Helper signatures and behavior of `REQ-0100`–`REQ-0104` | Tests | Required | Not started |
| The approved legacy x86 binary corpus resolves its runtime imports on x86 | Tests | Roadmap gate | Not started (needs an approved corpus) |

## Deferred Objectives

None yet.

## Change Control

Replan when a stage runs more than twice its forecast or an ADR changes a
decision recorded here.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0008 closeout record](MP-0008-closeout.md)
- [MP-0007 closeout record](MP-0007-closeout.md)
- [ADR-0007](../architecture/adr-0007-native-packaging-and-vbrun.md)
