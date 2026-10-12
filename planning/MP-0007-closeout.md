# MP-0007 — Native Compiler and VBRUN ABI: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0007 — Native compiler and VBRUN ABI (`0.7.0`)

**Status:** Complete (with documented deferrals)

**Completion date:** 2026-10-12

**Inherited baseline:** MP-0006 closeout (`MP-0006-closeout.md`)

**Tested baseline:** windows-x64-debug, 365 of 365 CTest tests passing; GitHub
Actions result for x86 and ARM64 to be recorded at approval

**Owner:** Project maintainer

**Approval:** Owner approval in session, 2026-10-12

## Outcome

MP-0007 delivers deployable packaged programs and the VBRUN export surface. It
does not deliver machine-code generation or working VBRUN helpers; the plan and
`ADR-0007` say so and allocate both to later work.

- **Packaged programs (`REQ-0303`).** `wfc --build project.vbp -o app.exe`
  writes a self-contained executable (a copy of the building `wfc` plus the
  project's files); it runs the project when started. The output has the
  architecture of the `wfc` that built it, so x86, x64 and ARM64 each have a
  deployment path. `TC-MP0007-package-*` compare packaged and plain runs.
- **VBRUN library (`REQ-0099`, `REQ-0105`).** `wfcvbrun.dll` has the
  `MSVBVM60` export directory (ordinal base 100, 1,911 slots, 635 named
  exports, empty slots left empty), generated from the retained inventory.
  Each export is a stub that names itself and ends the process with `0x5642`.
  `TC-MP0007-vbrun-library` parses the built PE, loads it, resolves exports by
  name and by ordinal, and calls a stub.

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| Packager and start-up detection | Complete | `REQ-0303`, `TC-MP0007-package-*` |
| Export library with reference layout | Complete | `REQ-0105`, `TC-MP0007-vbrun-library` |
| Loader test, stub diagnostic | Complete (no WFC diagnostic code: the stub is not a WFC component message) | `TC-MP0007-vbrun-library` |
| Behavior of `REQ-0100`–`REQ-0104` exports | Deferred: signatures need probes | MP-0009 |
| x86 helper process bridge on x64 and ARM64 | Withdrawn: owner decision, native code per architecture (`ADR-0007`) | MP-0009 |
| Machine-code generation, startup, unwind, layouts | Deferred | MP-0009 |
| Legacy x86 binary corpus | Deferred: no approved corpus | Reference machine |

## Exit Criteria

| Criterion | Evidence | Gate | Status |
| --- | --- | --- | --- |
| A packaged program runs and prints what the project prints | `TC-MP0007-package-*` | Required | Pass |
| The library export directory equals the reference layout | `TC-MP0007-vbrun-library` | Required | Pass |
| The library loads and unimplemented exports report their ordinal | `TC-MP0007-vbrun-library` | Required | Pass |
| Native machine-code generation | None | Roadmap gate | Not met; deferred |
| Legacy x86 binary corpus resolves imports | None | Roadmap gate | Not met; deferred |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| Machine-code generation: native x86, AMD64 and ARM64 back ends, no translation layer | Programs are interpreted inside the package | Project maintainer | MP-0009 | Packaged runtime | Pending |
| Export behavior and signatures | Native VB6 binaries do not run | Project maintainer | Reference machine | Stub diagnostics | Pending |
| Legacy x86 binaries on x64 and ARM64 | Not supported; no bridge or emulation | Project maintainer | Withdrawn | Native back ends (MP-0009) | Owner, 2026-10-12 |
| Packaged program arguments, files outside the project directory, signing | Some programs differ | Project maintainer | MP-0008 | `REQ-0303` limits | Pending |

## Closeout Decision

- **Decision:** Complete with documented deferrals
- **Approver:** Project maintainer
- **Rationale:** The two deliverables that can be verified without a VB6
  reference machine are done; the roadmap exit gate for native code and legacy
  binaries is not met and is recorded as such.
- **Next review or release action:** `MP-0008` (project compatibility); native compilation is `MP-0009`.

## References

- [MP-0007 plan](MP-0007-native-compiler.md)
- [Work log](work-log-mp-0007.md)
- [ADR-0007](../architecture/adr-0007-native-packaging-and-vbrun.md)
