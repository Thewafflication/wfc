# MP-0006 — `MSComctlLib` Compatibility

**Content type:** Milestone work plan  
**Status:** Complete (see closeout record)  
**Target baseline:** 0.6.0  
**Planned period:** 2026-10-12 through milestone closeout  
**Inherited baseline:** MP-0005 closeout (`MP-0005-closeout.md`), `master` at the closeout commit  
**Owner:** Project maintainer  
**Approval:** Plan drafted on the maintainer's instruction, 2026-10-12; formal approval pending

## Objective and Scope

VB6 projects that use the Microsoft Common Controls 6.0 (`MSCOMCTL.OCX`,
library `MSComctlLib` 2.0) run on every WFC target: TabStrip, Toolbar,
StatusBar, ProgressBar, TreeView, ListView, ImageList, Slider and ImageCombo,
with their collections, value objects, events and run-time behavior.

### Inherited (do not redo)

- **From MP-0004/MP-0005:** forms, the `UiHost` boundary, the generated control
  tables, the extender members (`Left`, `Top`, `Width`, `Height`, `Visible`,
  `TabIndex`, ...), `StdPicture`/`StdFont`, `PropertyBag`, `WithEvents`,
  control arrays, `Controls.Add`, form-file parser and writer.
- **`ADR-0006` (accepted 2026-10-12):** the nine controls are implemented
  natively in WFC behind the extender, registered under the `MSComctlLib`
  class names, so `Begin MSComctlLib.TabStrip` works without the 32-bit OCX on
  x86, x64 and ARM64. Hosting the stock OCX remains an x86-only test aid.

### Requirement set

| Group | Requirements | Disposition |
| --- | --- | --- |
| Control API contracts | `REQ-0001`–`REQ-0009` (members and events of the nine controls) | In scope |
| Behavior requirements | `REQ-0010`–`REQ-0036` (`TC-0010`–`TC-0036`) | In scope |
| Collections and value objects (`Tab`, `Panel`, `Button`, `ButtonMenu`, `Node`, `ListItem`, `ListSubItem`, `ColumnHeader`, `ListImage`, `ComboItem` and their collections) | Referenced by the contracts, no records yet | Specified here as `REQ-0296` onward |
| Persisted control state (`OleObjectBlob` in `.frx`) | `REQ-0115` | Text properties and WFC's own round trip in scope; the MSCOMCTL binary stream needs reference samples (see Risks) |

### Plan decisions

- **Table-driven API.** Property, method and event tables of the nine controls
  are generated from `evidence/reference/mscomctl-2.0-type-library.json` by the
  existing generator (`-ClassProperty controls`), merged at start-up with the
  container extender members, and checked by a CTest hash test. Collections and
  value objects are not in that evidence, so each has a small hand-written
  specification (properties, types, defaults, `Add` parameter list) kept in one
  place and documented in the requirement records.
- **One generic model.** Each control keeps its collections as ordered lists of
  property bags (`CItem`) with shared identity, so a `Node` or `ListItem`
  object stays valid while its collection changes; collection and item objects
  are native objects that read and write the bags with type coercion and the
  documented errors (key uniqueness 457, index 35600, invalid property 380).
- **Headless conformance first.** All behavior (selection, events, ordering,
  errors) runs on the headless host; scripted `ui*` input commands drive user
  actions. The Win32 host renders each control with the system common control
  (`SysTabControl32`, `ToolbarWindow32`, `msctls_statusbar32`,
  `msctls_progress32`, `SysTreeView32`, `SysListView32`, `msctls_trackbar32`,
  `ComboBoxEx32`) rebuilt from a model snapshot, and turns user input into the
  same events.
- **ImageList as a resource.** `ImageList` is a non-visual control owning
  pictures; other controls refer to it by `ImageList` property and
  index or key.

## Baseline and Assumptions

- No VB6 reference machine is available in the development environment, so
  behavior requirements stay `Proposed`; documented Microsoft behavior and the
  type-library evidence are the source of truth, and any behavior WFC had to
  choose is recorded in the requirement record.
- The 32-bit `MSCOMCTL.OCX` is not available to 64-bit processes.

## Deliverables

| Deliverable | Completion evidence |
| --- | --- |
| Generated `MSComctlLib` tables and hash check | Script, `comctl_tables.cpp`, CTest check |
| Object model: collections, value objects, errors | `REQ-0296`–`REQ-0304`, corpus tests |
| The nine controls with events and behavior | Corpus tests per control, `REQ-0010`–`REQ-0036` coverage matrix |
| Form loading of `Begin MSComctlLib.X` blocks and text properties | Corpus and round-trip tests |
| Win32 rendering and input | `wfc_win32_host_tests`, GUI screenshot test of all nine controls |
| Closeout record | `MP-0006-closeout.md` |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Framework | Tables, class registration, generic collections and items, `ImageList`, `ProgressBar`, `Slider` | Corpus tests for the three controls |
| 2. Strips | `TabStrip` and `StatusBar` | `REQ-0010`–`0012`, `0016`, `0017` tests |
| 3. Toolbar | Buttons, styles, groups, dropdowns, customization stubs | `REQ-0013`–`0015` tests |
| 4. TreeView | Nodes, relationships, expand/collapse, check, label edit, search | `REQ-0020`–`0023` tests |
| 5. ListView | Items, subitems, columns, views, sort, find | `REQ-0024`–`0027`, `0034` tests |
| 6. ImageCombo | Items, images, locking | `REQ-0033`, `0035`, `0036` tests |
| 7. Win32 | Rendering and input for all nine | Win32 host tests, screenshot evidence |
| 8. Persistence and closeout | Form round trip, diagnostics for unreadable state, closeout | All required gates pass |

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| `REQ-0001`–`REQ-0009` | Generated tables | Inventory audit in CTest | Yes |
| `REQ-0010`–`REQ-0036` | Per-control behavior | Corpus conformance tests, one per requirement group | Yes |
| `REQ-0296` onward | Collections and value objects | Corpus tests | Yes |
| Persistence | Form loader and writer | Round-trip tests | Yes |
| Rendering, DPI, themes | Win32 host | Win32 tests, screenshots | Partial |

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, decisions, completion evidence |
| Reviewer | Documented approved exception (single-maintainer project) | Review requirement records |
| Verifier | Project maintainer | Execute and retain build/test evidence; collect probes |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| The MSCOMCTL persistence stream (`OleObjectBlob`) is undocumented and no sample is available | Real forms start with empty collections | Load text properties, report a warning diagnostic naming the control when a blob is present, write a WFC text form of the collections for round trips, collect samples when a reference machine exists | Stage 8 |
| Collection semantics (keys, indexes, errors) differ in corners | Programs behave differently | Follow the Microsoft documentation, record every chosen behavior in the requirement records, keep probes planned | Per control |
| Common-control rendering differs from the OCX (themes, owner draw) | Visual differences | Accept system rendering; record variances | Stage 7 |
| Size of the work | Delay | Generic model and tables shared by all nine controls; commit each stage | Stage 1 |

## Estimate and Forecast

Forecast about 40–60 increments: 6–8 for the framework and simple controls,
8–12 each for TreeView and ListView, 4–6 each for TabStrip, Toolbar, StatusBar
and ImageCombo, 6–10 for Win32. Replan after stage 3.

## Execution and Evidence

- Local x64 debug configure, build and CTest per `AGENTS.md`; x86 and ARM64 are
  validated by GitHub Actions.
- Each increment adds corpus tests, a requirement record where applicable,
  diagnostics-catalog entries, and a work-log row, and is committed and pushed
  after local verification.
- Progress is recorded in [work-log-mp-0006.md](work-log-mp-0006.md).

## Rollback and Recovery

The controls live in their own source file included by the form runtime and
load only when a form names an `MSComctlLib` class. Revert a defective increment
as a normal Git change.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| API of the nine controls matches the type-library inventory | CTest hash and audit | Required | Pass |
| Behavior requirements `REQ-0010`–`REQ-0036` have passing WFC tests | Corpus tests | Required | Partial (REQ-0011, 0029, 0034) |
| Forms using the controls load their text properties and round trip | Round-trip tests | Required | Pass |
| Win32 host renders and drives the nine controls | Win32 tests, screenshots | Required | Pass |
| Unreadable persisted state is reported | Diagnostic test | Required | Pass |
| Behavioral probes retained | Probe records | Roadmap gate | Not met; deferred |

## Deferred Objectives

| Objective | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| MSCOMCTL binary persistence | Designer-saved collections do not load | Project maintainer | When samples exist (MP-0008) | Warning diagnostic; text round trip | Pending |
| VB6 reference probes | Behavior unconfirmed | Project maintainer | Reference machine before 1.0 | Conformance tests | Pending |
| Accessibility and theming beyond system common controls | Visual differences | Project maintainer | MP-0008 | Native controls | Pending |

## Change Control

Replan when a requirement is added or removed, a probe contradicts a Proposed
requirement, a stage runs more than twice its forecast, or reference samples of
the persisted stream become available.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0005 closeout record](MP-0005-closeout.md)
- [ADR-0006](../architecture/adr-0006-activex-hosting.md)
- [MSComctlLib requirements](../requirements/controls/README.md)
