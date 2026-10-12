# MP-0005 — Component Authoring, `stdole`, and OLE2: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0005 — Component authoring, `stdole`, and OLE2 (`0.5.0`)

**Status:** Proposed — ready for owner approval

**Inherited baseline:** MP-0004 closeout (`MP-0004-closeout.md`)

**Tested baseline:** windows-x64-debug, 352 of 352 CTest tests passing; GitHub
Actions (x86, x64, ARM64) green on the final commit

**Owner:** Project maintainer

**Approval:** Pending

## Outcome

WFC hosts ActiveX controls on forms, authors in-project `UserControl`s, serves
its own classes to other COM clients with an exported type library and events,
and provides the standalone `StdFont`, `StdPicture`, `PropertyBag`, `Licenses`,
`DataObject` and OLE container objects.

- **Control hosting (`ADR-0006`, `REQ-0293`).** `Object =` lines and
  `Begin Library.Class` form blocks resolve through type libraries; the Win32
  host creates each control in an ATL container window, the headless host creates
  it unsited. A container extender owns `Left`/`Top`/`Width`/`Height`/`Visible`/
  `TabIndex`/`Move`/`ZOrder` and the like and forwards everything else, including
  events, to the control. `Controls.Add` by ProgID, control arrays, `As
  VBControlExtender` and the OLE control shell (`CreateEmbed`, `Object`,
  `Close`) work.
- **`UserControl` (`REQ-0294`).** `.ctl` modules are hosted by forms of the
  project with their constituent controls, public members, events, `Initialize`/
  `ReadProperties`/`Resize`/`Show`/`Terminate`, `Extender`, `Ambient` and
  `PropertyChanged`; `Form_Initialize` now runs.
- **COM server (`ADR-0004`, `REQ-0295`).** Classes marked exposed are served
  in-process (`--serve`) or to other processes (`--server`, `--register`,
  `--unregister`); each object has `IDispatch`, an exported type library,
  `IConnectionPointContainer` and `IProvideClassInfo2`, so early-bound clients
  and `WithEvents` work across processes.
- **`stdole` and persistence (`REQ-0291`, `REQ-0292`).** Standalone `StdFont`
  (`Clone`, `IsEqual`, `hFont`, `FontChanged`) and `StdPicture` objects,
  `PropertyBag`, a form-file writer with a corpus round trip, `.frx` list items.
- **Forms and input carried from MP-0004.** Graphical button pictures,
  `Controls.Add`, `WithEvents` on intrinsic controls, mouse events, real mouse
  drag in the Win32 host, `DataObject` and a headless OLE drag sequence,
  `PrintForm`, system DPI awareness.

Principal limitations: no VB6 reference probes were run, so behavior
requirements remain `Proposed`; a `UserControl` cannot yet be exposed as an
ActiveX control to other containers (an OLE control must be an in-process
server, which needs the native compiler of MP-0007).

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| Carry-forward cleanup (button pictures, `.frx` lists, `Controls.Add`, `PrintForm`, Win32 mouse drag) | Complete; printer graphics deferred | `REQ-0289`, corpus 172–174, 184, 187 |
| `stdole` objects (`REQ-0095`, `REQ-0096`, `REQ-0098`) | Standalone objects complete; `Render`/`SaveAsFile` against a real DC, unqualified `OLE_TRISTATE`/`LoadPictureConstants` names deferred | `REQ-0291`, corpus 175 |
| Persistence (`REQ-0115` write side, property bags) | Property bag, form-file header writer and round trip complete; `.frx` writer and `OleObjectBlob` loading deferred | `REQ-0292`, corpus 176, `TC-MP0004-ui-units` |
| Control site and ActiveX container (`ADR-0006`) | Complete for `IDispatch` controls; blobs, ambient overrides, tab navigation deferred | `REQ-0293`, corpus 177–179, `TC-MP0005-gui-activex` |
| `VBControlExtender` (`REQ-0068`) | Type and extender complete; `ObjectEvent` deferred | `REQ-0293` |
| `UserControl` (`REQ-0064`), `PropertyPage` (`REQ-0065`) | In-project `UserControl` complete; `PropertyPage` build-time persistence and `InitProperties`/`WriteProperties` deferred | `REQ-0294`, corpus 180 |
| `Licenses` (`REQ-0067`) | Collection complete; licensed control creation deferred | corpus 183 |
| COM server and registration (`ADR-0004`) | Complete for late-bound and early-bound clients with events | `REQ-0295`, `TC-MP0005-com-server-*` |
| `UserControl` exposed as an ActiveX control (stage 7) | Deferred: needs an in-process server | MP-0007 |
| OLE container (`REQ-0063`, `REQ-0129`) and OLE drag and drop (`REQ-0114`) | Container shell, `DataObject` and a headless drag sequence complete; linked objects, dialogs, in-place UI and a real `IDropSource`/`IDropTarget` deferred | `REQ-0289`, `REQ-0293`, corpus 185, 186 |
| `UserDocument` (`REQ-0066`), OLE1 conversion | Deferred beyond 1.0 (disposition) | `legacy-feature-dispositions.md` |
| `MSComctlLib` strategy | `ADR-0006` proposes native controls on every architecture; owner approval needed before MP-0006 | `ADR-0006` |
| VB6 reference probes | Not run | Reference machine |

## Exit Criteria

| Criterion | Evidence | Gate | Status |
| --- | --- | --- | --- |
| `StdFont`, `StdPicture`, `LoadPicture`/`SavePicture` and constants match the `stdole` contracts | Corpus 175, `REQ-0291` | Required | Partial |
| A form hosts an ActiveX control: properties, methods, events | Corpus 177–179, `TC-MP0005-gui-activex` | Required | Pass (persistence blobs deferred) |
| WFC classes usable from another COM client and a WFC-authored control hostable | `TC-MP0005-com-server-*` | Required | Partial: classes pass; a hosted WFC control passes in-project only |
| `.frm`/`.frx` written by WFC reload to an equivalent form | `TC-MP0004-ui-units` round trip | Required | Partial: form block only |
| OLE2 container scenarios and OLE drag and drop | Corpus 185, 186 | Required | Partial |
| Removed features raise their diagnostics | `WFC0291` for unresolvable controls | Required | Pass |
| `ADR-0006` recorded and `MP-0006` plan baselined | `ADR-0006` | Required | `ADR-0006` Proposed; `MP-0006` plan not yet written |
| Behavioral probes retained | None | Roadmap gate | Not met; deferred |
| Diagnostics catalog current | CTest catalog check | Required | Pass |

## Verification Summary

| Configuration or method | Build or preparation | Verification | Evidence status |
| --- | --- | --- | --- |
| windows-x64-debug build and CTest | Pass | 352/352 | Complete |
| windows-x86, windows-arm64 | Built by GitHub Actions | Passed on the final commit | Complete |
| COM process tests | Per-user registration under HKCU | Pass; skipped (77) when it cannot be written | The development machine refused per-user `LocalServer32` activation, so the tests start the server themselves |
| GUI and Win32 host tests | Interactive desktop | Pass; the host reports itself unusable on a locked desktop | One flaky crash on a locked session was diagnosed and guarded |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| `UserControl` as an ActiveX control, in-process servers | WFC controls cannot be hosted by other containers | Project maintainer | MP-0007 | `ADR-0004` | Pending |
| `OleObjectBlob`/`.frx` persistence of hosted controls, `.frx` writer | Controls keep only text properties | Project maintainer | MP-0006 | Text properties applied | Pending |
| Real OLE drag and drop, linked OLE objects, dialogs | Drag between applications unavailable | Project maintainer | MP-0008 | Headless model | Pending |
| Printer graphics, `ObjectEvent`, licensed controls, `ITypeInfo` types beyond Variant | Some programs differ | Project maintainer | MP-0008 | Documented in the REQ records | Pending |
| VB6 reference probes | Behavior unconfirmed | Project maintainer | Reference machine before 1.0 | Conformance tests | Pending |

## Closeout Decision

- **Decision:** Pending owner approval
- **Proposed rationale:** All planned stages except the in-process control
  server are implemented and verified; the rest is allocated to later milestones
  or needs the reference machine.
- **Next review or release action:** On approval set the plan and README status
  to Closed, decide `ADR-0006`, and baseline `MP-0006`.

## References

- [MP-0005 plan](MP-0005-component-authoring.md)
- [Work log — MP-0005](work-log-mp-0005.md)
- [ADR-0004](../architecture/adr-0004-automation-server.md),
  [ADR-0005](../architecture/adr-0005-ui-platform.md),
  [ADR-0006](../architecture/adr-0006-activex-hosting.md)
