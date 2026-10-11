# MP-0004 — Forms and Intrinsic Controls

**Content type:** Milestone work plan  
**Status:** Planned  
**Target baseline:** 0.4.0  
**Planned period:** 2026-10-11 through milestone closeout  
**Inherited baseline:** MP-0003 closeout (`MP-0003-closeout.md`)  
**Owner:** Project maintainer  
**Approval:** Plan drafted on the maintainer's instruction, 2026-10-11; formal approval pending

## Objective and Scope

Ordinary and MDI VB6 form projects load from `.frm`/`.frx` files, display, take
keyboard and mouse input, run event procedures, draw, and save state, using the
intrinsic controls retained by the 1.0 profile. The same programs run with a
real Win32 window system and, for deterministic tests, with a scripted headless
backend.

### Inherited from MP-0003 (do not redo)

Classes, `Implements`, `WithEvents`, default instances (`VB_PredeclaredId`),
control-array-ready indexing in the class model, the `App`, `Clipboard`,
`Screen` and (empty) `Forms` objects, `LoadResString`/`LoadResData`, COM
Automation, and type-library references. A form is modelled as a class module
with a predeclared instance, so the existing class, event and lifetime
machinery is reused.

### Requirement set

| Group | Requirements | Disposition |
| --- | --- | --- |
| Control and form API contracts | `REQ-0037`–`REQ-0050`, `REQ-0053`–`REQ-0057`, `REQ-0059`–`REQ-0061` (PictureBox, Label, TextBox, Frame, CommandButton, CheckBox, OptionButton, ComboBox, ListBox, scroll bars, Timer, Printer, Form, drive/dir/file lists, Menu, MDIForm, Shape, Line, Image) | In scope |
| Behavior requirements | `REQ-0106`–`REQ-0127`, `REQ-0129`, `REQ-0131` | In scope; `REQ-0129` (OLE control) limited to the container shell, hosting is MP-0005 |
| `Data` control and DAO/Jet | `REQ-0062`, `REQ-0128` | Deferred to 2.0 (`legacy-feature-dispositions.md`) |
| DDE links | `REQ-0130` | Deferred beyond 1.0; members raise the specified unsupported error |
| `VBControlExtender`, `UserControl`, `PropertyPage`, `UserDocument` | `REQ-0064`–`REQ-0068` | MP-0005 (component authoring) |

### Carried forward from MP-0002 and MP-0003

`MsgBox` and `InputBox` dialogs (with the preserved help arguments), `SendKeys`,
live `Forms`, `Load`/`Unload`, `Printer`/`Printers`, and `Screen.ActiveForm`/
`ActiveControl`/`MouseIcon`.

### Plan decisions

- **Backend split (ADR-0005).** Forms and controls run on an abstract
  `UiHost` (windows, input, painting, timers, clipboard, dialogs). Two
  implementations: a Win32 host and a scripted headless host. The interpreter
  and the control model never call Win32 directly (architecture rule).
- **Table-driven object model.** Property, method and event tables (names,
  types, DISPIDs, defaults) for every control are generated from
  `evidence/reference/vb-6.0-type-library.json` by a script in `tools/`, not
  written by hand; behavior is coded per control family against the tables.
- **Native controls.** Windowed controls (TextBox, CommandButton, CheckBox,
  OptionButton, ListBox, ComboBox, scroll bars, Frame, PictureBox) map to Win32
  window classes through the Win32 host; lightweight controls (Label, Shape,
  Line, Image) are drawn by the form, as in VB6.
- **Minimal picture and font objects.** `Picture`, `Icon`, `MouseIcon`, `Font`
  and `LoadPicture` need bitmap/icon/cursor objects now. MP-0004 delivers
  `StdPicture`/`StdFont` behavior needed by the controls (load, size, render,
  `.frx` round trip); the full `stdole` contract, persistence functions and
  OLE conversions remain MP-0005.
- **Twips and DPI.** The coordinate model is twips-based with VB scale modes;
  the Win32 host converts at the monitor DPI. Per-monitor DPI awareness is
  declared in the manifest; layout results are expressed in twips so tests do
  not depend on the display.

## Baseline and Assumptions

- The reference VB6 environment (`reference-environment.md`) provides probe
  evidence for ordering and rendering questions. Probes that need a GUI run on
  the reference machine only; WFC conformance tests run headless in CTest, and a
  smaller Win32 integration set runs on the Windows runners.
- GitHub Actions Windows runners provide an interactive desktop; Win32-host
  tests create real windows and tear them down. If a runner lacks a desktop the
  tests report skipped, not passed.
- Rendering is checked by drawing-operation traces and selected pixel probes,
  not full-screen golden images, except where a requirement defines pixels.

## Deliverables

| Deliverable | Completion evidence |
| --- | --- |
| ADR-0005: UI platform abstraction and test hosts | Reviewed ADR |
| Control-table generator and generated tables | Script in `tools/`, CTest check that tables match the evidence JSON |
| `.frm`/`.frx` parser and form loader | Corpus and unit tests, round-trip persistence tests |
| Form/MDIForm model, lifecycle, modality, focus, validation, key preview | `TC-0106`–`TC-0111`, `TC-0115` conformance tests |
| Control families (stages 4–6) | Per-control conformance tests, `TC-0112`–`TC-0127` |
| Drawing and coordinate model (twips, scale modes, graphics methods) | `TC-0113` tests, drawing-trace tests |
| Win32 host | Windows integration tests on x86, x64 and ARM64 |
| Dialogs: `MsgBox`, `InputBox`, common message boxes; `SendKeys` | Headless script tests and Win32 tests |
| Printer and Printers | Spool-to-file test with the Microsoft Print to PDF device or a mock |
| Requirement records for each increment | `requirements/language/` entries, diagnostics catalog |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Platform | ADR-0005, `UiHost` interface, headless host, event queue, timers | Headless host drives a scripted click/key sequence in CTest |
| 2. Formats and tables | Generated control tables; `.frm`/`.frx` parser; `.vbp` `Form=`/`Startup` handling | A set of real `.frm` files parse and re-emit equivalently |
| 3. Form model | Form class with controls as members; `Initialize`/`Load`/`Activate` order; `Show`/`Hide`/`Unload`; `Forms` live; modality | `TC-0106`–`TC-0109` pass headless |
| 4. Basic controls | Label, TextBox, CommandButton, CheckBox, OptionButton, Frame, Timer with events and focus/tab order | `TC-0112`, `TC-0116`–`TC-0121`, `TC-0124` pass headless |
| 5. List and range controls | ListBox, ComboBox, scroll bars, drive/dir/file lists, Menu | `TC-0122`, `TC-0123`, `TC-0126`, `TC-0127` pass headless |
| 6. Graphics | Coordinate model, scale modes, `Line`/`Circle`/`PSet`/`Print`, AutoRedraw, PictureBox, Shape/Line/Image, `StdPicture` basics | `TC-0113`, `TC-0116`, `TC-0125` pass via drawing traces |
| 7. MDI and drag-drop | MDIForm, child forms, window arrangement, manual and OLE drag-drop | `TC-0110`, `TC-0114` pass |
| 8. Win32 host | Real windows, message loop, painting, DPI, accessibility names | Integration tests on all three architectures |
| 9. Interaction | `MsgBox`, `InputBox`, `SendKeys`, `Printer`, generic data binding (`REQ-0131`) | Tests for each; carried-forward items closed |
| 10. Persistence and closeout | Form/control persistence round trip (`REQ-0115`); closeout record | All required gates pass |

Stages 1–3 are the critical path. Stages 4–6 can interleave once a form loads.
Stage 8 may begin after stage 4 so the Win32 host grows with the controls
instead of arriving at the end.

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| `REQ-0106`–`REQ-0111`, `REQ-0115` | Form model, `.frm` loader | Headless conformance, round-trip tests | Yes |
| `REQ-0112`, `REQ-0116`–`REQ-0127` | Control families | Headless conformance plus reference probes | Yes |
| `REQ-0113`, `REQ-0125` | Drawing model | Drawing-trace tests; pixel probes where defined | Yes |
| `REQ-0114` | Drag-drop | Headless scripted drag; OLE drag-drop on Win32 | Yes |
| `REQ-0131` | Generic data binding | Binding conformance tests; no DAO/Jet | Yes |
| `REQ-0129` | OLE container shell | Create, size, save/load; hosting in MP-0005 | Partial |
| Control API contracts | Generated tables | Inventory audit against the type-library JSON | Yes |
| `MsgBox`, `InputBox`, `SendKeys`, `Printer` | Interaction layer | Headless and Win32 tests | Yes |

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, decisions, completion evidence |
| Reviewer | Documented approved exception (single-maintainer project) | Review ADR-0005 and requirement records |
| Verifier | Project maintainer | Execute and retain build/test evidence; collect reference probes |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| Event ordering and rendering details are undocumented | Wrong behavior accepted as correct | Reference probes first (`TC-0106`+); each behavior requirement stays Proposed until its probe result is retained | Per requirement |
| Interpreter re-parses text on each loop pass | Slow event loops and redraws | Keep event handlers short in tests; measure; speed work is MP-0008 | Stage 6 |
| Win32 tests are flaky on shared runners | Unreliable CI | Headless host carries the conformance suite; Win32 tests are few, serialized and time-bounded, skipped (not passed) without a desktop | Stage 8 |
| ARM64 and DPI differences | Layout drift | Layout in twips; DPI conversions isolated in the host | Stage 8 |
| Scope growth from `.frx` binary resources and pictures | Delay | Support the resource types retained controls need; defer exotic types with diagnostics | Stage 2 |
| Re-entrancy: event handlers run inside the message loop | Corrupted interpreter state | Reuse the COM-event re-entrancy path (`deliver_com_event`); add stress tests for nested `DoEvents` and modal loops | Stage 3 |
| Accessibility and themes are undefined by the type library | Missed requirement | Use standard Win32 controls (themed, UIA-capable); record variances | Stage 8 |

## Estimate and Forecast

No size data exists for UI work. Stages 1–3 are forecast at roughly 25 small
increments; each control family at 10–20; the Win32 host at 15–25. Replan after
stage 3 using measured increment size and again when stage 8 starts.

## Execution and Evidence

- Local x64 debug configure, build and CTest per `AGENTS.md`; x86 and ARM64 are
  validated by GitHub Actions.
- Each increment adds corpus or conformance tests, a requirement record where
  applicable, diagnostics-catalog entries, and a work-log row, and is committed
  and pushed after local verification.
- Progress is recorded in [work-log-mp-0004.md](work-log-mp-0004.md).

## Rollback and Recovery

The UI code lives behind the `UiHost` interface in its own targets. Revert a
defective increment as a normal Git change; programs without forms are
unaffected because form support loads only when a project lists forms.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| Every retained behavior requirement has VB6 reference evidence and a passing WFC test | Reference probe records plus CTest | Required | Not started |
| Retained control and form API contracts match the type-library inventory | Inventory audit in CTest | Required | Not started |
| Real forms load from `.frm`/`.frx`, run and persist | Corpus and round-trip tests | Required | Not started |
| Win32 host passes its integration set on x86, x64 and ARM64 | GitHub Actions | Required | Not started |
| Carried-forward items closed (`MsgBox`, `InputBox`, `SendKeys`, `Printer`, live `Forms`, `Load`/`Unload`) | Tests | Required | Not started |
| Removed legacy controls and behaviors raise their specified diagnostics (`Data`, DDE) | Tests | Required | Not started |
| Diagnostics catalog current | CTest catalog check | Required | Ongoing |

## Deferred Objectives

| Objective | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| `Data` control and DAO/Jet | No database-bound forms | Project maintainer | 2.0 | Unsupported-feature diagnostic | Decided 2026-10-09 |
| DDE links | No DDE | Project maintainer | Beyond 1.0 | Unsupported-feature diagnostic | Decided 2026-10-09 |
| Full `stdole` font/picture contracts, OLE hosting, `UserControl`, `PropertyPage` | Controls limited to retained intrinsic set | Project maintainer | MP-0005 | Minimal `StdPicture`/`StdFont` | Pending |
| `MSComctlLib` controls | Common controls absent | Project maintainer | MP-0006 | None | Pending |
| Interpreter speed | Slow redraw-heavy programs | Project maintainer | MP-0008 | Short handlers in tests | Pending |

## Change Control

Replan when the `UiHost` boundary changes, a requirement is added or removed, a
reference probe contradicts a Proposed requirement, or a new third-party
dependency (for example an image codec) is introduced.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0003 closeout record](MP-0003-closeout.md)
- [ADR-0005](../architecture/adr-0005-ui-platform.md)
- [Form and intrinsic-control behavioral requirements](../requirements/vb/behavior/README.md)
- [Visual Basic class requirements](../requirements/vb/classes/README.md)
- `wsp/processes/milestone-plan-template.md`
