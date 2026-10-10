# MP-0002 — Core VB/VBA Language Execution: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0002 — Core VB/VBA language execution (`0.2.0`)

**Status:** Complete (with documented deferrals)

**Completion date:** 2026-10-10

**Inherited baseline:** `89aff7f` — "Pre-authorize local cmake/ctest/wfc runs"

**Completed baseline:** `master` at `4a8784e` plus the commit that adds this record

**Tested baseline:** `4a8784e`, windows-x64-debug, 292 of 292 CTest tests passing

**Owner:** Project maintainer

**Approval:** Owner approval in session, 2026-10-10, on condition that the
carried-forward limits are allocated to later milestones (done below)

## Outcome

Non-visual VB6 programs, including multi-module `.vbp` projects with class
modules, execute with VB6-equivalent values, conversions, control flow, errors,
files, dates, strings, arrays, Variants, and the VBA standard library. The
corpus has 138 programs; every callable member in the `REQ-0069`–`REQ-0079`
inventories and all 118 constants in `REQ-0080`–`REQ-0094`/`REQ-0098` were
audited (see the work log's "Closeout audit"). Function behavior was verified by
curated programs and unit tests, not by a side-by-side run against a real VB6
runtime for every member.

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| Language, grammar, semantic, and diagnostic specifications | Complete | `requirements/language/`, `diagnostics.md` (catalog check in CTest) |
| Procedures, module state, expressions, statements, arrays, UDTs, Variants, strings, errors, conditional compilation | Complete | Corpus 1–138, `evaluator_tests.cpp` |
| VBA library `REQ-0069`–`REQ-0094` | Complete (`MacScript` excluded: Macintosh only) | Work log "Closeout audit" |
| `.vbp`, `.bas`, module dependency loading | Complete | `REQ-0283`, corpus 114 |
| Minimal class-module foundation `REQ-0200`, `REQ-0203`–`REQ-0206` | Complete; extended with `.cls` loading, `Implements`, `WithEvents`, predeclared instances | `REQ-0283`; MP-0003 re-scoped |
| Disposition of DDE, DAO, OLE1, ActiveX Documents, PropertyPage, help | Complete | `legacy-feature-dispositions.md` |
| Per-module `Option Explicit` | Deferred | MP-0003 |
| `VB_GlobalNameSpace` class members | Deferred | MP-0003 |
| String-valued `#Const` symbols | Deferred | MP-0003 |
| `MsgBox`/`InputBox` dialogs, real `SendKeys` | Deferred | MP-0004 |
| Interpreter execution speed | Deferred | MP-0008 (native compiler in MP-0007 supersedes the interpreter hot path) |

## Exit Criteria

| Criterion | Evidence | Gate | Status |
| --- | --- | --- | --- |
| Core-language corpus passes on the initial target | CTest 292/292, windows-x64-debug | Required | Pass |
| Applicable VBA requirements pass | Closeout audit; corpus; unit tests | Required | Pass |
| Diagnostics stable and documented | `check_diagnostic_catalog.cmake` in CTest | Required | Pass |
| No legacy candidate needed by MP-0003/MP-0004 lacks a disposition | `legacy-feature-dispositions.md` | Required | Pass |

## Verification Summary

| Configuration or method | Build or preparation | Verification | Evidence status |
| --- | --- | --- | --- |
| windows-x64-debug build and CTest | Pass | 292/292 | Complete |
| windows-x86, windows-arm64 | N/A locally | Validated by GitHub Actions after push, per repository policy | Not re-checked at closeout |
| Mutation fuzzing of the corpus (Release) | Pass | No crashes remaining; timeouts were infinite loops in mutated programs | Informative |

## Review and Finding Summary

| Review or finding | Disposition | Evidence, approval, or condition |
| --- | --- | --- |
| Uncaught `bad_variant_access`, unbounded recursion, and native stack overflow found by fuzzing | Resolved | `WFC0900`, `WFC0123` guards; corpus and unit tests |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| Per-module `Option Explicit`; `VB_GlobalNameSpace`; string `#Const` | Rare project layouts are rejected or merged more loosely than VB6 | Project maintainer | MP-0003 | Documented in `REQ-0283` | Owner, 2026-10-10 |
| `MsgBox`/`InputBox` UI, `SendKeys` | Programs run headless; dialogs are no-ops | Project maintainer | MP-0004 | Documented limit | Owner, 2026-10-10 |
| Interpreter speed (about 1 µs per simple statement, Release) | Long loops are slow | Project maintainer | MP-0008 | None needed for the corpus | Owner, 2026-10-10 |

## Lessons and Process Improvement

- Probing realistic multi-file programs found far more defects than extending
  per-function arity evidence; later milestones should start from end-to-end
  programs.
- Fuzzing the corpus paid for itself (three crashes); keep it in the workflow.

## Handoff and Release Effect

MP-0003 builds on the delivered class/project work (`REQ-0283`) and is planned
in `MP-0003-classes-projects-automation.md`. No product release is cut at this
baseline.

## Closeout Decision

- **Decision:** Complete with documented deferrals
- **Approver:** Project maintainer
- **Required gates:** All Pass
- **Rationale:** Required gates pass; deferrals are allocated to later
  milestones.
- **Next review or release action:** MP-0003 plan baseline

## References

- [Work log — MP-0002](work-log-mp-0002.md)
- [Roadmap to 1.0.0](roadmap-1.0.md)
- [Legacy feature dispositions](legacy-feature-dispositions.md)
