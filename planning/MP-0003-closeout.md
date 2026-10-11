# MP-0003 — Classes, Projects, and Automation: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0003 — Classes, projects, and Automation (`0.3.0`)

**Status:** Proposed (awaiting owner decision)

**Completion date:** Not yet approved

**Inherited baseline:** MP-0002 closeout (`MP-0002-closeout.md`)

**Completed baseline:** `master` at the commit that adds this record

**Tested baseline:** windows-x64-debug, 304 of 304 CTest tests passing; GitHub Actions green for x86, x64 and ARM64 through the COM-events commit

**Owner:** Project maintainer

**Approval:** Pending

## Outcome

Multi-file projects with class modules, interfaces, events, default members,
enumerators, global-namespace classes, per-module `Option Explicit`, string
conditional constants, `.res` resources and the headless `App`, `Clipboard`,
`Screen` and `Forms` objects run end to end. The interpreter is also a
late-bound and early-bound COM Automation client: `CreateObject`/`GetObject`,
members, default members, bang access, named and ByRef arguments, `For Each`,
errors, identity, type-library references (classes, interfaces, constants) and
`WithEvents` over connection points all work against stock Windows components.

Principal limitation: WFC classes cannot yet be consumed as COM objects
(`ADR-0004` defers the server to MP-0005), so the exit gate's "exchange objects
in both directions" is met for the client direction only.

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| Class-language completeness (default members, enumerators, `Friend`, conformance errors, interface `Set`) | Complete | `REQ-0284`, corpus 149, existing class corpus |
| Per-module `Option Explicit`, `VB_GlobalNameSpace`, string `#Const` | Complete | `REQ-0283`, corpus 139, 143, 144 |
| `App`, `Clipboard`, `Screen`, `Forms`, `LoadResString`, `LoadResData` | Complete (non-visual) | `REQ-0285`, `REQ-0287`, corpus 140, 141, 145, 146 |
| Automation client | Complete | `REQ-0286`, corpus 142, 148, 150 |
| Type-library references and early binding | Complete | `REQ-0288`, corpus 147 |
| COM identity | Complete | `REQ-0286`, corpus 142 |
| Automation server decision | Complete (ADR) | `ADR-0004` |
| Automation server implementation | Deferred | MP-0005 |
| `Load`/`Unload`, `LoadPicture`/`SavePicture`, `Printer`/`Printers`, `Licenses` | Deferred | MP-0004/MP-0005 |
| `VBControlExtender` (`REQ-0068`) | Deferred | MP-0004 |
| Class inheritance | Removed (VB6 has none) | Plan correction |

## Exit Criteria

| Criterion | Evidence | Gate | Status |
| --- | --- | --- | --- |
| Multi-file projects with classes, interfaces, events, default members and enumerators run end to end | Corpus 114–150 | Required | Pass |
| Language limits carried from MP-0002 closed | Corpus 139, 143, 144 | Required | Pass |
| Non-visual service-object members present | Corpus 140, 141, 145, 146 | Required | Pass |
| Late-bound client works against stock COM objects | Corpus 142, 147, 148, 150 | Required | Pass |
| Server decision recorded | `ADR-0004` | Required | Pass |
| Diagnostics catalog current | CTest catalog check | Required | Pass |
| Objects exchanged with an external WFC Automation server | Not applicable until the server exists | Roadmap gate | Not met; deferred by `ADR-0004` |

## Verification Summary

| Configuration or method | Build or preparation | Verification | Evidence status |
| --- | --- | --- | --- |
| windows-x64-debug build and CTest | Pass | 304/304 | Complete |
| windows-x86, windows-arm64 | Built by GitHub Actions | Passed through the events commit | Later pushes not re-checked |
| COM tests use `WScript.Shell`, `MSXML2.DOMDocument.6.0`, `ADODB.Recordset` | Present on supported Windows | Pass | Verified on one host only |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| COM server hosting | WFC classes cannot be used from other COM clients | Project maintainer | MP-0005 | `ADR-0004` design of record | Pending |
| Pictures, printing, forms-dependent globals | `LoadPicture`, `Printer` etc. unavailable | Project maintainer | MP-0004/MP-0005 | `Forms` is empty | Pending |
| COM tests depend on host components | A stripped Windows image could fail them | Project maintainer | MP-0008 | Components ship with Windows | Pending |

## Closeout Decision

- **Decision:** Pending. Recommended: Complete with documented deferrals.
- **Approver:** Project maintainer
- **Rationale:** All planned work except the deferred server is complete and
  verified; the roadmap gate wording about two-way object exchange is met for
  the client side and deferred for the server by `ADR-0004`.
- **Next review or release action:** Baseline the MP-0004 plan.

## References

- [MP-0003 plan](MP-0003-classes-projects-automation.md)
- [Work log — MP-0003](work-log-mp-0003.md)
- [ADR-0004](../architecture/adr-0004-automation-server.md)
