# MP-0001 — Foundation and Feasibility

**Content type:** Milestone work plan  
**Status:** In progress
**Target baseline:** 0.1.0
**Planned period:** 2026-08-26 through initial baseline approval  
**Inherited baseline:** Repository initial state; WSP submodule commit recorded in Git  
**Owner:** Project maintainer  
**Approval:** Pending

## Objective and Scope

Establish the controlled project baseline and prove the technical direction for
WFC: a C++ reimplementation of the VB6 product and runtime surface, including
VB, VBA, VBRUN, `stdole`, and `MSComctlLib`, for Windows x86, x64, and ARM64.

This milestone produces the decisions, build skeleton, and one small
end-to-end feasibility path needed to begin feature delivery with confidence.
It does not attempt broad VB6 compatibility or a usable compiler release.

### Included Work

- Adopt and tailor the Waughtal Software Process (WSP) baseline.
- Establish the C++ toolchain, CMake build, warnings policy, formatting and
  static-analysis approach, and target matrix.
- Define the initial component boundary between the VB/VBA front end, runtime,
  COM/OLE interoperation, and common controls.
- Identify the compatibility reference corpus and the first supported behavior.
- Implement and verify one thin, vertical feasibility slice: source input,
  parsing or representation, runtime execution, and an observable result.
- Create the initial requirements, architecture, test, and evidence structure.

### Excluded or Deferred Work

- Full VB6 or VBA language compatibility.
- Binary compatibility with existing VB6 executables or project files.
- Complete `stdole` automation and `MSComctlLib` control implementations.
- IDE, form designer, debugger, installer, code signing, and public release.
- Support for platforms other than Windows x86, x64, and ARM64.

## Baseline and Assumptions

- WSP is adopted as the `wsp/` submodule; its immutable release and pinned
  commit will be recorded before this milestone closes.
- WCRT will be used as the native C++ runtime/build foundation. Its selected
  revision, license, supplied facilities, and integration mode require an ADR.
- CMake is the initial build-system choice; supported compiler versions and
  dependency policy require confirmation.
- Compatibility claims will be evidence-based and introduced incrementally by
  documented language and library subsets.
- The initial implementation target is Windows. Cross-architecture behavior is
  a required design concern even where all targets are not yet continuously
  built.

## Deliverables

| Deliverable | Owner | Completion evidence |
| --- | --- | --- |
| WSP adoption record and profile dispositions | Project maintainer | Reviewed controlled record; submodule pin agrees |
| Initial product requirements and compatibility-scope statement | Project maintainer | Requirement review and traceability allocation |
| Architecture overview and ADRs for product boundaries, WCRT, and toolchain | Project maintainer | ADR review |
| CMake build skeleton for supported architectures | Project maintainer | Clean configure and build evidence |
| Vertical feasibility slice | Project maintainer | Automated test demonstrating observable result |
| Initial test strategy and test/evidence dispatchers | Project maintainer | Review and successful sample execution |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Baseline | WSP and WCRT adoption decisions are explicit | Adoption record and WCRT ADR are reviewed |
| 2. Scope | First compatibility claim is narrow and verifiable | Initial requirements and non-goals are baselined |
| 3. Architecture | Component ownership and interfaces are understood | Architecture overview and decisive ADRs exist |
| 4. Build | A reproducible native build works for the initial host target | Configure, build, and warning policy are captured |
| 5. Slice | A minimal source-to-result path proves the design | Automated end-to-end test passes |
| 6. Closeout | Evidence and next-milestone input are retained | Review, test report, and closeout record are approved |

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| WSP adoption baseline | `planning/wsp-adoption-record.md` | Inspection | Yes |
| Reproducible native build | Top-level CMake and build documentation | Clean configure/build on host target | Yes |
| Defined compatibility boundary | Requirements and architecture overview | Review | Yes |
| WCRT integration decision | ADR | ADR review and build demonstration | Yes |
| Thin vertical slice | Front-end and runtime components | Automated end-to-end test | Yes |
| Initial architecture target matrix | Architecture overview | Inspection | Yes |

The [0.1.0 foundation requirement set](../requirements/foundation/README.md)
replaces these objective-level allocations with `REQ-0132` through `REQ-0139`.

The first allocation is the [MSComctlLib control requirement set](../requirements/controls/README.md),
which captures the statically observable Automation contracts. Behavioral
requirements remain planned until reference probes provide objective results.

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, decisions, and completion evidence |
| Reviewer | Assigned reviewer or documented approved exception | Review requirements, ADRs, and source changes |
| Verifier | Project maintainer or independent verifier | Execute and retain build/test evidence |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| VB6 behavior is underspecified or differs across versions | Incorrect compatibility claims | Define subsets; use curated reference programs and documented observations | Before each compatibility claim |
| WCRT constraints conflict with product needs | Architectural rework | Record integration assumptions and prove them in the vertical slice | Before build foundation closes |
| COM/OLE ABI differences across architectures | Runtime incompatibility | Design ABI boundaries explicitly; add architecture-targeted tests before exposing interop claims | Before COM/OLE scope enters a milestone |
| Scope expands from a runtime slice into a full IDE | Delayed usable baseline | Treat non-slice features as separately approved milestones | On new feature requests |
| Toolchain warnings or non-reproducible builds | Defects and delivery risk | Establish build matrix and warnings policy early | During build setup |

## Estimate and Forecast

No reliable historical effort or size data exists yet. Estimates will be added
after the vertical slice establishes component size, build duration, and test
execution measures. Replan when a required deliverable requires a new product
area, an unplanned external dependency, or a material change to the first
compatibility boundary.

## Execution and Evidence

- Retain configure, build, static-analysis, and test output by architecture.
- Use the recorded [VB6 discovery baseline](reference-environment.md) for
  type-library inspection and behavioral probes.
- Keep requirements, ADRs, test specifications, and review records in the
  repository with stable identifiers.
- Use a top-level test dispatcher once the build skeleton exists; preserve
  failing output with the corresponding change or defect record.
- Record the exact WSP and WCRT revisions used for each reviewed milestone.

## Rollback and Recovery

Keep experimental components isolated behind build targets or feature-specific
libraries. Revert a defective feasibility slice as a normal Git change while
retaining its test output and decision records. Do not alter historical
evidence; supersede it with a corrected record where needed.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| WSP adoption is recorded | Adoption record and matching gitlink | Required | Implemented; review pending |
| WCRT use is explicit and justified | Reviewed ADR | Required | Documented; linked proof pending |
| Initial compatibility boundary is defined | Reviewed requirement set | Required | Documented; review pending |
| Architecture boundaries and target matrix are documented | Architecture overview and ADRs | Required | Documented; open decisions remain |
| Initial build is repeatable | Successful configure/build record | Required | Local x86/x64/ARM64 build passed; retained evidence pending |
| Vertical feasibility slice executes under automated test | Passing retained test result | Required | Local x86/x64 tests passed; retained evidence pending |
| Next milestone is proposed with any deferrals | Closeout and successor plan | Required | Planned |

## Deferred Objectives

| Objective | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| Full language compatibility | No broad compatibility claim in this milestone | Project maintainer | MP-0002 through MP-0009 | Explicit supported-subset documentation | Pending |
| COM/OLE and common-control compatibility | No interop or control compatibility claim | Project maintainer | MP-0003 through MP-0006 | Isolated interfaces and deferred tests | Pending |
| Multi-architecture continuous builds | Initial evidence may cover host target only | Project maintainer | MP-0008 | Architecture-aware design and target matrix | Pending |

## Change Control

Replan and assess requirements and architecture impact when the first supported
VB/VBA subset changes, WCRT is replaced or materially reconfigured, a new
third-party dependency is added, COM/OLE compatibility is claimed, or a target
architecture is promoted to a required build gate. Record durable decisions as
ADRs.

## References

- [WFC 1.0 compatibility profile](compatibility-profile-1.0.md)
- [Roadmap to 1.0.0](roadmap-1.0.md)
- `wsp/processes/milestone-plan-template.md`
- `wsp/processes/software-lifecycle.md`
- `wsp/requirements/adoption-record-template.md`
