# MP-0007 — Native Compiler and VBRUN ABI

**Content type:** Milestone work plan  
**Status:** Planned  
**Target baseline:** 0.7.0  
**Planned period:** 2026-10-12 through milestone closeout  
**Inherited baseline:** MP-0006 closeout (`MP-0006-closeout.md`), `master` at the closeout commit  
**Owner:** Project maintainer  
**Approval:** Plan drafted on the maintainer's instruction, 2026-10-12; formal approval pending

## Objective and Scope

WFC builds deployable programs for x86, x64 and ARM64 and ships a library whose
export directory matches `MSVBVM60.DLL`, so that the approved legacy x86 binary
corpus can resolve its runtime imports.

### Inherited (do not redo)

- The interpreter, forms, controls, COM server and project loader (MP-0002 to
  MP-0006). A packaged program runs this engine.
- The retained reference inventory
  `evidence/reference/msvbvm60-6.0-runtime.json` and `REQ-0099`–`REQ-0105`.

### Requirement set

| Requirement | Subject | Disposition |
| --- | --- | --- |
| `REQ-0099` | Binary identity and loader | Library builds and loads; cross-architecture bridge deferred |
| `REQ-0105` | Export-directory layout | In scope; verified by parsing the built DLL |
| `REQ-0100`–`REQ-0104` | Export categories (engine, compiler helpers, language runtime, member/memory, numeric) | Names and ordinals in scope; behavior only where a signature is confirmed (stubs elsewhere) |
| `REQ-0303` (new) | Packaged native programs | In scope |
| Native code generation | Roadmap principal work | Deferred (see `ADR-0007` and Risks) |

### Plan decisions

See [ADR-0007](../architecture/adr-0007-native-packaging-and-vbrun.md): packaged
programs now, a generated export surface, an explicit x86 bridge deferral.

## Baseline and Assumptions

- No VB6 compiler or native VB6 binary is available in the development
  environment, so signatures and behavior cannot be probed; stubs report the
  ordinal they were reached through.
- The packager copies the running image, so a build made by the x64 binary is an
  x64 program.

## Deliverables

| Deliverable | Completion evidence |
| --- | --- |
| `wfc --build` and start-up payload detection | `REQ-0303`, CTest packaging tests |
| `wfcvbrun.dll` with the reference export directory | Generator script, CTest layout check against the JSON |
| Loader test | Load by name and ordinal from a test executable |
| Stub diagnostics | Catalog entry, test |
| Closeout record | `MP-0007-closeout.md` |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Planning | ADR, plan, requirement | Documents baselined |
| 2. Packager | `--build`, payload, run | Packaged programs match plain runs |
| 3. Export library | Generated `.def`, stubs, DLL | Layout test passes |
| 4. Loader | Load, resolve by name and ordinal, stub diagnostic | Test passes |
| 5. Closeout | Coverage and deferrals recorded | Required gates pass |

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| `REQ-0105` | Generated module definition | Export directory parsed and compared with the JSON | Yes |
| `REQ-0099` | CMake target `wfc_vbrun` | PE header inspection, load test | Yes |
| `REQ-0303` | `src/package.cpp` | Build, run and compare | Yes |
| `REQ-0100`–`REQ-0104` | Stubs | Name and ordinal checks | Partial |

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, decisions, completion evidence |
| Reviewer | Documented approved exception (single-maintainer project) | Review requirement records |
| Verifier | Project maintainer | Execute and retain build/test evidence |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| Export signatures are unknown | Native binaries cannot run | Stubs name the reached ordinal; probes planned for a reference machine | Stage 4 |
| No machine-code generator | Programs are interpreted inside the package | State this plainly in the closeout; keep `--build` as the stable interface | Closeout |
| The extraction directory is writable by others | Tampering | Per-payload directory under the user's own temporary directory, contents verified by hash before reuse | Stage 2 |

## Estimate and Forecast

About 10–15 increments for the stages above; machine-code generation would be a
separate milestone of its own.

## Execution and Evidence

- Local x64 debug configure, build and CTest per `AGENTS.md`; x86 and ARM64 are
  validated by GitHub Actions, where the x86 job also loads the library.
- Progress is recorded in [work-log-mp-0007.md](work-log-mp-0007.md).

## Rollback and Recovery

The packager and library are separate sources and targets; revert a defective
increment as a normal Git change.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| A packaged program runs and prints what the project prints | Packaging tests | Required | Pass |
| The library export directory equals the reference layout | Layout test | Required | Pass |
| The library loads and unimplemented exports report their ordinal | Loader test | Required | Pass |
| Native machine-code generation | None | Roadmap gate | Not met; deferred |
| Legacy x86 binary corpus resolves imports | None | Roadmap gate | Deferred (no corpus) |

## Deferred Objectives

| Objective | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| Machine-code generation | Programs are interpreted inside the package | Project maintainer | After 0.9 or a new milestone | Packaged runtime | Pending |
| Export behavior and signatures | Native VB6 binaries do not run | Project maintainer | Reference machine | Stub diagnostics | Pending |
| x86 helper process bridge on x64 and ARM64 | No legacy binary runs there | Project maintainer | MP-0008 | None | Pending |

## Change Control

Replan when a signature probe becomes possible, a reference binary corpus is
approved, or machine-code generation is scheduled.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0006 closeout record](MP-0006-closeout.md)
- [ADR-0007](../architecture/adr-0007-native-packaging-and-vbrun.md)
- [VBRUN requirements](../requirements/vbrun/README.md)
