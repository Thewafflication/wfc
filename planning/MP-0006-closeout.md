# MP-0006 — `MSComctlLib` Compatibility: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0006 — `MSComctlLib` compatibility (`0.6.0`)

**Status:** Complete (with documented deferrals)

**Completion date:** 2026-10-12

**Inherited baseline:** MP-0005 closeout (`MP-0005-closeout.md`)

**Tested baseline:** windows-x64-debug, 362 of 362 CTest tests passing; GitHub
Actions (x86, x64, ARM64) green on the "Refine common control behaviors" commit

**Owner:** Project maintainer

**Approval:** Owner approval in session, 2026-10-12

## Outcome

TabStrip, Toolbar, StatusBar, ProgressBar, TreeView, ListView, ImageList, Slider
and ImageCombo are implemented natively (`ADR-0006`), so `Begin MSComctlLib.X`
forms load and run on x86, x64 and ARM64 without the 32-bit OCX.

- **Object model (`REQ-0296`).** Generic collections and value objects (`Tab`,
  `Panel`, `Button`, `ButtonMenu`, `Node`, `ListItem`, `ListSubItem`,
  `ColumnHeader`, `ListImage`, `ComboItem`) with shared identity, key and index
  rules and the documented errors.
- **Controls (`REQ-0297`–`REQ-0301`, `REQ-0010`–`REQ-0036`).** Behavior and
  events per control, driven on the headless host by scripted `ui` commands;
  see [MP-0006-coverage.md](MP-0006-coverage.md) for the per-requirement matrix.
- **Win32 host.** Each control is rendered by the system common control rebuilt
  from a model snapshot, with `ImageList` images, and user input returns as the
  same events (`TC-MP0006-gui-comctl`, screenshot evidence).
- **Persistence (`REQ-0302`).** Text properties and WFC's own collection groups
  load; an unreadable `OleObjectBlob` raises warning `WFC0293`.
- **Tables.** Generated from the type-library evidence with a CTest hash check.

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| Generated tables and hash check | Complete | `TC-MP0006-comctl-tables-current` |
| Object model, errors | Complete | `REQ-0296`, corpus 188–195 |
| Nine controls, `REQ-0010`–`REQ-0036` | Complete headless; partials listed in the coverage matrix (`REQ-0011`, `REQ-0029`, `REQ-0034`) | Corpus 188–195 |
| Form loading and round trip | Complete for text properties and WFC groups | Corpus 194 |
| Win32 rendering and input | Complete with system rendering | `TC-MP0006-gui-comctl` |
| MSCOMCTL binary persistence | Deferred: needs reference samples | Warning `WFC0293` |
| VB6 reference probes | Not run | Reference machine |

## Exit Criteria

| Criterion | Evidence | Gate | Status |
| --- | --- | --- | --- |
| API matches the type-library inventory | CTest hash and audit | Required | Pass |
| `REQ-0010`–`REQ-0036` have passing WFC tests | Corpus 188–195 | Required | Partial (three requirements) |
| Forms load their text properties and round trip | Corpus 194 | Required | Pass |
| Win32 host renders and drives the nine controls | `TC-MP0006-gui-comctl` | Required | Pass |
| Unreadable persisted state is reported | Corpus 194, `WFC0293` | Required | Pass |
| Behavioral probes retained | None | Roadmap gate | Not met; deferred |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| MSCOMCTL binary persistence | Designer-saved collections do not load | Project maintainer | When samples exist (MP-0008) | Warning diagnostic; text round trip | Pending |
| `ImageList.Draw` styles, overlay mask composite, ListView hover selection | Minor visual differences | Project maintainer | MP-0008 | Properties and `ExtractIcon` work | Pending |
| Cancelled label edit passes an empty string rather than `Null` | A program testing `IsNull` differs | Project maintainer | Reference machine | `Cancel` and recorded difference | Pending |
| Accessibility, theming, DPI beyond system controls | Visual differences | Project maintainer | MP-0008 | Native controls | Pending |
| VB6 reference probes | Behavior unconfirmed | Project maintainer | Reference machine before 1.0 | Conformance tests | Pending |

## Closeout Decision

- **Decision:** Complete with documented deferrals
- **Approver:** Project maintainer
- **Rationale:** All nine controls work end to end; the remaining items need a
  reference machine or binary samples.
- **Next review or release action:** `MP-0007-native-compiler.md`.

## References

- [MP-0006 plan](MP-0006-mscomctllib.md)
- [Coverage matrix](MP-0006-coverage.md)
- [Work log](work-log-mp-0006.md)
- [ADR-0006](../architecture/adr-0006-activex-hosting.md)
