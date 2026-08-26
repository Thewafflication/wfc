# REQ-0009 — ImageCombo Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`ImageCombo` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `ImageCombo` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.ImageComboCtl.2` |
| Reference CLSID | `{DD9DA666-8594-11D1-B16A-00C0F0283628}` |
| Default interface | `IImageCombo`, `{DD9DA664-8594-11D1-B16A-00C0F0283628}` |
| Default event interface | `DImageComboEvents`, `{DD9DA665-8594-11D1-B16A-00C0F0283628}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `BackColor` | Get/Let | `ULong` | -501 | — |
| `ComboItems` | Get/Set | `IComboItems*` | 7 | — |
| `Enabled` | Get/Let | `Boolean` | -514 | — |
| `Font` | Get/Let/Set | `Font*` | -512 | — |
| `ForeColor` | Get/Let | `ULong` | -513 | — |
| `hWnd` | Get/Let | `Integer` | -515 | NonBrowsable |
| `ImageList` | Get/Set/Let | `Object` | 3 | — |
| `Indentation` | Get/Let | `Integer` | 4 | — |
| `Locked` | Get/Let | `Boolean` | 9 | — |
| `MouseIcon` | Get/Let/Set | `Picture*` | 10 | — |
| `MousePointer` | Get/Let | `MousePointerConstants` | 11 | — |
| `OLEDragMode` | Get/Let | `OLEDragConstants` | 1550 | — |
| `OLEDropMode` | Get/Let | `OLEDropConstants` | 1551 | — |
| `SelectedItem` | Get/Set/Let | `IComboItem*` | 14 | — |
| `SelLength` | Get/Let | `Long` | 16 | NonBrowsable |
| `SelStart` | Get/Let | `Long` | 17 | NonBrowsable |
| `SelText` | Get/Let | `String` | 18 | NonBrowsable |
| `Text` | Get/Let | `String` | -517 | Bindable, RequestEdit |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AboutBox` | `` | `Void` | -552 | Hidden |
| `GetFirstVisible` | `` | `IComboItem*` | 30 | — |
| `Refresh` | `` | `Void` | -550 | — |
| `OLEDrag` | `` | `Void` | 1552 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Change` | `` | 1 |
| `Dropdown` | `` | 2 |
| `Click` | `` | -600 |
| `KeyDown` | `KeyCode: Integer, Shift: Integer` | -602 |
| `KeyUp` | `KeyCode: Integer, Shift: Integer` | -604 |
| `KeyPress` | `KeyAscii: Integer*` | -603 |
| `OLEStartDrag` | `ByRef Data: DataObject**, ByRef AllowedEffects: Long*` | 1550 |
| `OLEGiveFeedback` | `ByRef Effect: Long*, ByRef DefaultCursors: Boolean*` | 1551 |
| `OLESetData` | `ByRef Data: DataObject**, ByRef DataFormat: Integer*` | 1552 |
| `OLECompleteDrag` | `ByRef Effect: Long*` | 1553 |
| `OLEDragOver` | `ByRef Data: DataObject**, ByRef Effect: Long*, ByRef Button: Integer*, ByRef Shift: Integer*, ByRef x: Single*, ByRef y: Single*, ByRef State: Integer*` | 1554 |
| `OLEDragDrop` | `ByRef Data: DataObject**, ByRef Effect: Long*, ByRef Button: Integer*, ByRef Shift: Integer*, ByRef x: Single*, ByRef y: Single*` | 1555 |

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- Combo-item collection creation, removal, lookup, images, and indentation.
- Text editing, selection, locking, dropdown, scrolling, colors, and fonts.
- Keyboard, selection, dropdown, and OLE drag-and-drop event behavior.
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
