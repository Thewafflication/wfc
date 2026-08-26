# REQ-0008 — Slider Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`Slider` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `Slider` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.Slider.2` |
| Reference CLSID | `{F08DF954-8592-11D1-B16A-00C0F0283628}` |
| Default interface | `ISlider`, `{F08DF952-8592-11D1-B16A-00C0F0283628}` |
| Default event interface | `ISliderEvents`, `{F08DF953-8592-11D1-B16A-00C0F0283628}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Value` | Get/Let | `Long` | 0 | Hidden |
| `BorderStyle` | Get/Let | `BorderStyleConstants` | -504 | — |
| `Enabled` | Get/Let | `Boolean` | -514 | Bindable, RequestEdit |
| `GetNumTicks` | Get | `Long` | 15 | — |
| `hWnd` | Get/Let | `Integer` | -515 | NonBrowsable |
| `LargeChange` | Get/Let | `Long` | 1 | — |
| `Max` | Get/Let | `Long` | 3 | — |
| `Min` | Get/Let | `Long` | 4 | — |
| `MouseIcon` | Get/Let/Set | `Picture*` | 12 | — |
| `MousePointer` | Get/Let | `MousePointerConstants` | 13 | — |
| `OLEDropMode` | Get/Let | `OLEDropConstants` | 1551 | — |
| `Orientation` | Get/Let | `OrientationConstants` | 5 | — |
| `SelectRange` | Get/Let | `Boolean` | 6 | — |
| `SelLength` | Get/Let | `Long` | 8 | — |
| `SelStart` | Get/Let | `Long` | 7 | — |
| `SmallChange` | Get/Let | `Long` | 2 | — |
| `Text` | Get/Let | `String` | 16 | NonBrowsable |
| `TextPosition` | Get/Let | `TextPositionConstants` | 17 | — |
| `TickFrequency` | Get/Let | `Long` | 10 | — |
| `TickStyle` | Get/Let | `TickStyleConstants` | 9 | — |
| `Value` | Get/Let | `Long` | 11 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Refresh` | `` | `Void` | -550 | — |
| `ClearSel` | `` | `Void` | 14 | — |
| `DoClick` | `` | `Void` | -551 | Hidden |
| `OLEDrag` | `` | `Void` | 1552 | — |
| `AboutBox` | `` | `Void` | -552 | Hidden |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Click` | `` | -600 |
| `KeyDown` | `KeyCode: Integer*, Shift: Integer` | -602 |
| `KeyPress` | `KeyAscii: Integer*` | -603 |
| `KeyUp` | `KeyCode: Integer*, Shift: Integer` | -604 |
| `MouseDown` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -605 |
| `MouseMove` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -606 |
| `MouseUp` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -607 |
| `Scroll` | `` | 1 |
| `Change` | `` | 2 |
| `OLEStartDrag` | `ByRef Data: DataObject**, ByRef AllowedEffects: Long*` | 1550 |
| `OLEGiveFeedback` | `ByRef Effect: Long*, ByRef DefaultCursors: Boolean*` | 1551 |
| `OLESetData` | `ByRef Data: DataObject**, ByRef DataFormat: Integer*` | 1552 |
| `OLECompleteDrag` | `ByRef Effect: Long*` | 1553 |
| `OLEDragOver` | `ByRef Data: DataObject**, ByRef Effect: Long*, ByRef Button: Integer*, ByRef Shift: Integer*, ByRef x: Single*, ByRef y: Single*, ByRef State: Integer*` | 1554 |
| `OLEDragDrop` | `ByRef Data: DataObject**, ByRef Effect: Long*, ByRef Button: Integer*, ByRef Shift: Integer*, ByRef x: Single*, ByRef y: Single*` | 1555 |

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- Minimum, maximum, value, small-change, and large-change range semantics.
- Ticks, selection range, orientation, text, appearance, and programmatic clicks.
- Keyboard, pointer, scroll, change, and OLE drag-and-drop event ordering.
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
