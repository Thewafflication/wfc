# MP-0004 — Forms and Intrinsic Controls: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0004 — Forms and intrinsic controls (`0.4.0`)

**Status:** Proposed — ready for owner approval (work continues until approval)

**Inherited baseline:** MP-0003 closeout (`MP-0003-closeout.md`)

**Tested baseline:** windows-x64-debug, 333 of 333 CTest tests passing; GitHub
Actions (x86, x64, ARM64) green through the data-binding commit

**Owner:** Project maintainer

**Approval:** Pending

## Outcome

Ordinary and MDI form projects load from `.frm`/`.frx` files, show, take input,
run event procedures and draw, with the same programs running on a scripted
headless host (the CTest conformance suite) and on a real Win32 host
(`--gui`, `--screenshots`). Forms are class modules with predeclared instances;
controls are native objects whose property, method and event tables are
generated from the type-library JSON. Implemented: the `Load`/`Show`/`Hide`/
`Unload`/`End` lifecycle with modal forms, control arrays and dynamic controls,
the `Controls` and `Forms` collections, focus, tab order, default/cancel
buttons, accelerators, validation, `KeyPreview`, `Timer`, `ListBox`,
`ComboBox`, scroll bars, menus and pop-up menus, `Font`, scale modes and the
surface drawing methods, `Point`, `PaintPicture`, pictures, `Image`/`Shape`/
`Line`, `MsgBox`/`InputBox`, `SendKeys`, MDI forms, manual drag and drop,
the virtual `Printer`/`Printers`, drive/directory/file list boxes and generic
data binding to class-module sources. Unsupported forms are rejected at load
with `WFC0291`/`WFC0292`.

Principal limitation: no VB6 reference probes were run (no reference machine
in this environment), so behavior requirements remain `Proposed` and the
evidence is WFC conformance tests plus the Win32 integration and screenshot
tests.

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| `UiHost`, headless host, event queue, timers (ADR-0005) | Complete | `wfc_ui_tests`, corpus 151–171 |
| Generated control tables with an evidence hash check | Complete | `ui-tables-current` test, `scripts/New-UiControlTables.ps1` |
| `.frm`/`.frx` parser and loader, `Form=`/`Startup` in `.vbp` | Complete | `REQ-0289`, `REQ-0115` (load-time diagnostics) |
| Form model, lifecycle, modality, focus, validation, key preview | Complete | `REQ-0289`, corpus 151–163 |
| Basic, list and range controls, menus | Complete | `REQ-0289`, corpus 151–159 |
| Drive, directory and file list boxes (`REQ-0126`) | Complete (attribute filters not applied) | `REQ-0289`, corpus 170 |
| Drawing, scale modes, `Print`, `Point`, `PaintPicture`, pictures | Complete except the items below | `REQ-0290`, `REQ-0291`, corpus 160–166 |
| MDI forms | Complete except menu merging | `REQ-0289`, corpus 167, `TC-MP0004-gui-mdi` |
| Manual drag and drop (`REQ-0114`) | Complete headless | `REQ-0289`, corpus 168 |
| OLE drag and drop, mouse-driven drag on Win32 | Deferred | MP-0005 (OLE) |
| Win32 host (windows, controls, menus, timers, dialogs, `SendInput`, screenshots, DPI) | Complete (system DPI only) | `wfc_win32_host_tests`, `TC-MP0004-gui-*` |
| `MsgBox`, `InputBox`, `SendKeys` | Complete | corpus 155, 164, `TC-MP0004-win32-host` |
| `Printer`, `Printers` | Virtual printer with spooled text; graphics, `PrintForm`, device output deferred | `REQ-0289`, corpus 169 |
| Generic data binding (`REQ-0131`) | Complete for class-module sources; DAO/Jet excluded | `REQ-0289`, corpus 171 |
| Persistence round trip (`REQ-0115`) | Load and diagnostics complete; WFC does not write `.frm` files | `REQ-0289`, TC-MP0004-form-* |
| `DrawStyle` dashes, hatch fills, `DrawMode`, `AutoRedraw` semantics | Deferred (repainting clears program drawings) | MP-0008 |
| Command/option button pictures, list items from `.frx`, `Controls.Add` | Deferred | MP-0005 |
| OLE container (`REQ-0129`) | Shell only (property storage); hosting deferred | MP-0005 |
| `VBControlExtender`, `UserControl`, `PropertyPage` | Deferred (moved from MP-0003's note) | MP-0005 |
| VB6 reference probes (`TC-0106` onward) | Not run | Reference machine; requirements stay Proposed |
| Per-monitor DPI, accessibility names | Deferred | MP-0008 |

## Exit Criteria

| Criterion | Evidence | Gate | Status |
| --- | --- | --- | --- |
| Ordinary and MDI forms load, display, take input and run events | Corpus 151–171, `TC-MP0004-gui-*` | Required | Pass |
| Retained intrinsic controls present with API tables matching the type library | `ui-tables-current`, member audit | Required | Pass |
| Headless suite and Win32 integration tests pass | CTest 333/333 | Required | Pass |
| Diagnostics catalog current | CTest catalog check | Required | Pass |
| Behavioral probes retained for each behavior requirement | None | Roadmap gate | Not met; deferred |
| Persistence round trip with every control class | Load tests only | Required | Partial |

## Verification Summary

| Configuration or method | Build or preparation | Verification | Evidence status |
| --- | --- | --- | --- |
| windows-x64-debug build and CTest | Pass | 333/333 | Complete |
| windows-x86, windows-arm64 | Built by GitHub Actions | Passed through the data-binding commit | Later pushes not re-checked |
| Win32 host tests and GUI screenshots | Interactive desktop required | Pass; skip (77) without a desktop | Verified on one host; PNGs uploaded by CI |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| VB6 reference probes | Ordering and rendering details unconfirmed | Project maintainer | Reference machine before 1.0 | Conformance tests and screenshots | Pending |
| OLE drag and drop, OLE container hosting, `VBControlExtender` | ActiveX and OLE programs cannot run | Project maintainer | MP-0005 | `WFC0291` rejects them at load | Pending |
| Printer graphics and `PrintForm` | Printing programs limited to text | Project maintainer | MP-0005 | Spooled text output | Pending |
| Drawing fidelity (`DrawStyle`, `DrawMode`, `AutoRedraw`) | Some graphics differ | Project maintainer | MP-0008 | Documented in `REQ-0290` | Pending |
| MDI menu merging | Child menus do not merge into the frame | Project maintainer | MP-0008 | Documented in `REQ-0289` | Pending |

## Closeout Decision

- **Decision:** Pending owner approval
- **Proposed rationale:** All planned stages are implemented and verified; the
  deferrals above are allocated to later milestones or require the VB6
  reference machine.
- **Next review or release action:** On approval set the plan and README status
  to Closed and begin MP-0005.

## References

- [MP-0004 plan](MP-0004-forms-and-controls.md)
- [Work log — MP-0004](work-log-mp-0004.md)
- [ADR-0005](../architecture/adr-0005-ui-platform.md)
