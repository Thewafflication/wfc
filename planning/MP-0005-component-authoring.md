# MP-0005 — Component Authoring, `stdole`, and OLE2

**Content type:** Milestone work plan  
**Status:** Planned  
**Target baseline:** 0.5.0  
**Planned period:** 2026-10-11 through milestone closeout  
**Inherited baseline:** MP-0004 closeout (`MP-0004-closeout.md`), `master` at the closeout commit  
**Owner:** Project maintainer  
**Approval:** Plan drafted on the maintainer's instruction, 2026-10-11; formal approval pending

## Objective and Scope

WFC programs can host ActiveX controls on forms, author and register ActiveX
components (classes, controls), expose their own public classes to other COM
clients, and use the standard OLE font, picture and persistence contracts with
the COM identity, events, errors and lifetime of the reference implementation.
The same scenarios run against deterministic in-process test components in CTest
and against stock Windows components on the reference host.

### Inherited (do not redo)

- **From MP-0003:** the Automation client (`IDispatch`, `VARIANT` marshalling,
  identity-preserving wrapper registry, connection-point sinks, type-library
  reading) — the reusable half of `ADR-0004`'s server design.
- **From MP-0004:** the `UiHost` abstraction, Win32 and headless hosts, the
  generated control tables, forms and intrinsic controls, minimal
  `StdPicture`/`StdFont` (`REQ-0291`), `LoadPicture`/`SavePicture` for image
  files, the virtual `Printer`, load-time rejection of unsupported controls
  (`WFC0291`), manual drag and drop, and the generic data-binding shell.

### Requirement set

| Group | Requirements | Disposition |
| --- | --- | --- |
| `stdole` contracts | `REQ-0095` (`StdFont`), `REQ-0096` (`StdPicture`), `REQ-0097` (`LoadPicture`/`SavePicture`), `REQ-0098` (constants) | In scope (full contracts) |
| Component classes | `REQ-0064` (`UserControl`), `REQ-0065` (`PropertyPage`), `REQ-0068` (`VBControlExtender`), `REQ-0067` (`Global`, including `Licenses`) | In scope; `PropertyPage` per its disposition: accept and persist at build time, no runtime hosting UI |
| OLE container | `REQ-0063` (`OLE` control), `REQ-0129` (OLE container behavior), `REQ-0114` (OLE drag and drop) | In scope for OLE2; OLE1 conversion (`SaveToOle1File`) deferred beyond 1.0 |
| `UserDocument` | `REQ-0066` | Deferred beyond 1.0 (disposition 4); the class raises the specified unsupported-feature error |
| Persistence | `REQ-0115` (writing side), property bags, `ReadProperties`/`WriteProperties`/`InitProperties` | In scope |
| Server hosting | `ADR-0004` option 2 (out-of-process local server hosted by `wfc.exe`) | In scope; in-process DLL servers wait for MP-0007 |
| New requirements | `REQ-0292` onward (see Work Breakdown) | To be written per increment |

### Carried forward from MP-0004

OLE drag and drop and mouse-driven drag on Win32, the `OLE` container beyond
property storage, `VBControlExtender`, `UserControl`/`PropertyPage`, command and
option button pictures (`DownPicture`, `DisabledPicture`), list items loaded
from `.frx`, `Controls.Add`, printer graphics and `PrintForm`, and the
persistence round trip (writing `.frm`/`.frx`). Items that need no COM work
(button pictures, `.frx` lists, `Controls.Add`) are scheduled first because they
unblock control hosting tests.

### Plan decisions

- **ActiveX control container.** A form line such as `Begin MSComctlLib.TabStrip`
  is today rejected with `WFC0291`. This milestone adds a control site: the
  control is created with `CoCreateInstance`, sited through `IOleObject`/
  `IOleClientSite`/`IOleControlSite`, activated in place in a Win32 window
  provided by the `UiHost`, given ambient properties, and wrapped as a native
  control object whose members and events go through `IDispatch` and connection
  points (reusing the MP-0003 machinery). The headless host sites test controls
  without a window. Controls that are not registered keep producing `WFC0291`
  with the class name.
- **`MSComctlLib` strategy is decided here, executed in MP-0006.** Hosting the
  stock `MSCOMCTL.OCX` versus a WFC implementation of the nine controls is
  recorded in `ADR-0006` before MP-0006 starts, using what the container can
  and cannot host (licensing, 32-bit-only OCX on x64/ARM64, theming). This
  milestone proves the container with test controls and, where installed, one
  stock OCX.
- **Server hosting follows `ADR-0004`.** `wfc --server project.vbp` registers
  class factories with `CoRegisterClassObject`, serves calls on the interpreter
  apartment through a dynamic `IDispatch` built from `ClassDef`, and exports an
  `ITypeInfo` synthesized from the public members for early binding.
  `wfc --register`/`--unregister` write per-user keys only on explicit request;
  tests use in-process registration and never touch the registry.
- **`UserControl` in two steps.** First a `UserControl` module used by forms of
  the same project (events, `Extender`, `Ambient`, property bag persistence,
  lifetime events `Initialize`/`InitProperties`/`ReadProperties`/`Show`/
  `WriteProperties`/`Terminate`), then exposure as an ActiveX control to other
  containers through the server and the control site.
- **Full `stdole`.** `StdFont` and `StdPicture` become real COM coclasses with
  `IFontDisp`/`IPictureDisp`, `IFont`/`IPicture`, `Render`, `hPal`,
  `FontChanged`, `SaveAsFile`, and the `OLE_COLOR`/`OLE_HANDLE`-style types; the
  MP-0004 minimal objects are replaced behind the same VB-visible names.
- **Win32 real input drag.** Mouse-driven manual drag (`DragMode`, drag icon,
  `DragOver` states) and OLE drag and drop share one drag loop in the Win32
  host; the headless script gains `oledrag` for tests.
- **No registry changes by tests.** Registration is exercised through
  registration-free paths (in-process class objects, activation contexts) and a
  per-user registration test that cleans up after itself and is skipped on
  runners that block it.

## Baseline and Assumptions

- Test components are built by the repository: a small in-process ActiveX
  control and an out-of-process server written in C++ under `tests/fixtures/`,
  so container and server tests are deterministic and need no installed
  software. Stock components (`MSComctlLib` where present, `StdPicture`,
  `StdFont`, `WScript.Shell`) are used for identity and compatibility checks and
  reported as skipped when absent.
- 32-bit-only OCX files cannot load into x64 or ARM64 processes; those tests are
  x86-only and reported as skipped elsewhere, not passed.
- Reference probes need the VB6 reference machine; as in MP-0004 the evidence is
  WFC conformance tests until probe results are retained, and behavior
  requirements stay `Proposed`.

## Deliverables

| Deliverable | Completion evidence |
| --- | --- |
| `ADR-0006`: ActiveX control hosting and the `MSComctlLib` strategy | Reviewed ADR |
| Full `StdFont`, `StdPicture`, `LoadPicture`/`SavePicture`, constants | `REQ-0095`–`REQ-0098` conformance tests, COM identity tests |
| Control site and ActiveX container (headless and Win32) | Container tests with the repository test control; one stock-OCX smoke test where installed |
| `VBControlExtender`, `Extender`, `Ambient`, `Licenses` | Tests per member table |
| `UserControl` authoring and in-project use; `PropertyPage` persistence | Corpus tests, build-time persistence tests |
| Property bags and persistence both ways; `.frm`/`.frx` writer | Round-trip tests over every control class |
| Out-of-process server, type-information export, registration commands | In-process and out-of-process client tests, per-user registration test |
| OLE container control and OLE2 drag and drop | Test server scenarios, scripted and Win32 drag tests |
| Carried-forward items (button pictures, `.frx` lists, `Controls.Add`, printer graphics, `PrintForm`) | Corpus tests |
| Requirement records and diagnostics-catalog entries for each increment | `requirements/language/` entries, catalog check |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Carry-forward cleanup | Button pictures, `.frx` list items, `Controls.Add`, printer graphics and `PrintForm`, Win32 mouse drag | Corpus and Win32 tests pass |
| 2. `stdole` | Full `StdFont`/`StdPicture` coclasses, `Render`, `SaveAsFile`, constants, full `LoadPicture`/`SavePicture` | `REQ-0095`–`REQ-0098` tests pass |
| 3. Persistence | Property bag, `ReadProperties`/`WriteProperties`, `.frm`/`.frx` writer, round trip for every control class | Round-trip tests over all intrinsic classes |
| 4. Control site | ADR-0006, `CoCreateInstance` sited controls, ambient properties, in-place activation, events, `VBControlExtender`, `Extender` | Test control loads from a form, raises events, persists |
| 5. `UserControl` | In-project authoring, lifetime events, property pages persisted | Corpus tests |
| 6. Server and registration | `wfc --server`, class factories, dynamic `IDispatch`, exported type information, `--register`/`--unregister`, `Licenses` | Out-of-process client exercises a WFC class, early and late bound |
| 7. ActiveX control authoring | `UserControl` exposed to other containers through stage 4 and 6 | WFC container hosts a WFC-authored control through COM |
| 8. OLE container | `OLE` control, embedded/linked test objects, verbs, persistence, OLE2 drag and drop | Test-server scenarios pass |
| 9. Closeout | Evidence, deferrals, closeout record, `MP-0006` plan | All required gates pass |

Stages 1–3 are independent of COM hosting and are done first; stage 4 is the
critical path for MP-0006.

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| `REQ-0095`–`REQ-0098` | `stdole` objects | Conformance tests, COM identity checks, inventory audit against the type library | Yes |
| `REQ-0064`, `REQ-0065`, `REQ-0068`, `REQ-0067` | Component classes and `Global` | Member-table audit plus behavior tests | Yes |
| `REQ-0063`, `REQ-0129`, `REQ-0114` | OLE container and drag and drop | Test-server scenarios; Win32 drag tests | Yes |
| `REQ-0115` (write side) | `.frm`/`.frx` writer | Round trip over all control classes | Yes |
| `ADR-0004` server | `wfc --server` | In-process and out-of-process client tests | Yes |
| `REQ-0066`, `SaveToOle1File` | Unsupported-feature diagnostics | Tests | Yes |
| Behavior probes | Reference machine | Retained probe records | Roadmap gate; may defer with approval |

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, decisions, completion evidence |
| Reviewer | Documented approved exception (single-maintainer project) | Review `ADR-0006` and requirement records |
| Verifier | Project maintainer | Execute and retain build/test evidence; collect reference probes |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| In-place activation and `IOleObject` sites vary widely by control | Hosted controls misbehave | Start with the repository test control, add stock controls one at a time, record unsupported interfaces | Stage 4 |
| Re-entrant calls into the single-threaded interpreter from COM clients | Corrupted state or deadlock | Apartment-affine server, reuse `deliver_com_event` path, stress tests with nested `DoEvents` | Stage 6 |
| Registration affects the test machine | Polluted CI hosts | Registration-free tests, explicit-only registry commands, cleanup and skip logic | Stage 6 |
| 32-bit OCX files on x64/ARM64 | Control gaps on 64-bit targets | `ADR-0006` records the policy; tests skip rather than pass | Stage 4 |
| Type-information export is large | Delay | Export only public members and enums first; defer exotic attributes with diagnostics | Stage 6 |
| Interpreter speed under event-heavy controls | Slow forms | Keep handlers short in tests; speed is MP-0008 | Stage 4 |
| Licensing of design-time-only controls | Controls refuse to load | `Licenses` support and an explicit unsupported diagnostic | Stage 4 |

## Estimate and Forecast

MP-0004 averaged about 27 increments; component hosting has fewer user-visible
behaviors but more interop risk. Forecast roughly 8–10 increments for stages
1–3, 12–18 for the control site and extender, 8–12 for `UserControl`, 10–15 for
the server and registration, and 8–12 for OLE. Replan after stage 4.

## Execution and Evidence

- Local x64 debug configure, build and CTest per `AGENTS.md`; x86 and ARM64 are
  validated by GitHub Actions.
- Each increment adds corpus or conformance tests, a requirement record where
  applicable, diagnostics-catalog entries, and a work-log row, and is committed
  and pushed after local verification.
- Progress is recorded in [work-log-mp-0005.md](work-log-mp-0005.md).

## Rollback and Recovery

Component code lives in its own targets behind the interpreter's native-object
interface and the `UiHost` boundary. Revert a defective increment as a normal Git
change; projects without components are unaffected because the control site and
server load only when a form names an ActiveX control or the command line asks
for `--server`.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| `StdFont`, `StdPicture`, `LoadPicture`/`SavePicture` and constants match the `stdole` contracts | CTest and inventory audit | Required | Not started |
| A form hosts an ActiveX control: properties, methods, events, persistence | Container tests | Required | Not started |
| WFC classes and a WFC-authored control are usable from another COM client and from a WFC container | Server and container tests | Required | Not started |
| `.frm`/`.frx` written by WFC reload to an equivalent form for every control class | Round-trip tests | Required | Not started |
| OLE2 container scenarios and OLE drag and drop pass | Test-server scenarios | Required | Not started |
| Removed features (`UserDocument`, OLE1 conversion) raise their specified diagnostics | Tests | Required | Not started |
| `ADR-0006` recorded and `MP-0006` plan baselined | Documents | Required | Not started |
| Behavioral probes retained for each behavior requirement | Probe records | Roadmap gate | Not started |
| Diagnostics catalog current | CTest catalog check | Required | Ongoing |

## Deferred Objectives

| Objective | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| `UserDocument` / ActiveX Documents | None for 1.0 | Project maintainer | Beyond 1.0 | Unsupported-feature diagnostic | Decided 2026-10-09 |
| OLE1 conversion (`SaveToOle1File`) | None for 1.0 | Project maintainer | Beyond 1.0 | Unsupported-feature diagnostic | Decided 2026-10-09 |
| In-process DLL/OCX servers | Components run out of process only | Project maintainer | MP-0007 | `ADR-0004` option 2 | Pending |
| `MSComctlLib` controls | Common controls absent | Project maintainer | MP-0006 | Container proven here; `ADR-0006` decides strategy | Pending |
| Drawing fidelity (`DrawStyle`, `DrawMode`, `AutoRedraw`), MDI menu merging, per-monitor DPI, accessibility names | Some visual differences | Project maintainer | MP-0008 | Documented in `REQ-0289`/`REQ-0290` | Pending |
| VB6 reference probes carried from MP-0004 | Behavior unconfirmed | Project maintainer | Reference machine before 1.0 | Conformance tests | Pending |

## Change Control

Replan when `ADR-0006` changes the container boundary, a requirement is added or
removed, a reference probe contradicts a Proposed requirement, a stock OCX
cannot be hosted as assumed, or a new third-party dependency is introduced.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0004 closeout record](MP-0004-closeout.md)
- [ADR-0004](../architecture/adr-0004-automation-server.md)
- [ADR-0005](../architecture/adr-0005-ui-platform.md)
- [Legacy feature dispositions](legacy-feature-dispositions.md)
- [REQ-0291](../requirements/language/req-0291-pictures.md)
