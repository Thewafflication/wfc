# REQ-0004 — ProgressBar Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`ProgressBar` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `ProgressBar` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.ProgCtrl.2` |
| Reference CLSID | `{35053A22-8589-11D1-B16A-00C0F0283628}` |
| Default interface | `IProgressBar`, `{35053A20-8589-11D1-B16A-00C0F0283628}` |
| Default event interface | `IProgressBarEvents`, `{35053A21-8589-11D1-B16A-00C0F0283628}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Appearance` | Get/Let | `AppearanceConstants` | -520 | — |
| `BorderStyle` | Get/Let | `BorderStyleConstants` | -504 | — |
| `ControlDefault` | Get/Let | `Single` | 0 | Hidden, NonBrowsable |
| `Enabled` | Get/Let | `Boolean` | -514 | — |
| `hWnd` | Get | `Integer` | -515 | NonBrowsable |
| `Max` | Get/Let | `Single` | 1 | — |
| `Min` | Get/Let | `Single` | 2 | — |
| `MouseIcon` | Get/Set/Let | `Picture` | 4 | — |
| `MousePointer` | Get/Let | `MousePointerConstants` | 3 | — |
| `OLEDropMode` | Get/Let | `OLEDropConstants` | 1551 | — |
| `Orientation` | Get/Let | `OrientationConstants` | 6 | — |
| `Scrolling` | Get/Let | `ScrollingConstants` | 7 | — |
| `Value` | Get/Let | `Single` | 5 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `OLEDrag` | `` | `Void` | 1552 | — |
| `AboutBox` | `` | `Void` | -552 | Hidden |
| `Refresh` | `` | `Void` | -550 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `MouseDown` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -605 |
| `MouseMove` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -606 |
| `MouseUp` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -607 |
| `Click` | `` | -600 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 1550 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 1551 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 1552 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 1553 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single, ByRef State: Integer` | 1554 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single` | 1555 |

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- Minimum, maximum, and current-value range semantics including invalid values.
- Horizontal and vertical layout, scrolling mode, appearance, and borders.
- Pointer and OLE drag-and-drop event behavior.
- Default values, boundary values, invalid inputs, error numbers, persistence,
  rendering, focus, keyboard navigation, and behavior at common DPI settings.

## Rationale

Matching the published Automation surface is necessary for source-level VB6
compatibility and provides a stable boundary for later behavioral probes.

## Verification

**Method:** Inspection

**References:** `evidence/reference/mscomctl-2.0-type-library.json`; test case TBD

Inspect the WFC type library and compare every listed member name, DISPID,
access mode, signature, and event against this requirement. Runtime semantics
are verified by separate behavioral requirements and tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** WCRT integration ADR and COM identity/coexistence ADR
- **Conflicts with:** None known

## Tailoring

A member may be deferred only through an approved compatibility-scope change
that identifies the resulting limitation.

## Implementation Record

Not implemented.
