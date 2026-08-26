# REQ-0001 — TabStrip Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`TabStrip` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `TabStrip` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.TabStrip.2` |
| Reference CLSID | `{1EFB6596-857C-11D1-B16A-00C0F0283628}` |
| Default interface | `ITabStrip`, `{1EFB6594-857C-11D1-B16A-00C0F0283628}` |
| Default event interface | `ITabStripEvents`, `{1EFB6595-857C-11D1-B16A-00C0F0283628}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `ClientHeight` | Get/Let | `Single` | 11 | NonBrowsable |
| `ClientLeft` | Get/Let | `Single` | 10 | NonBrowsable |
| `ClientTop` | Get/Let | `Single` | 9 | NonBrowsable |
| `ClientWidth` | Get/Let | `Single` | 12 | NonBrowsable |
| `Enabled` | Get/Let | `Boolean` | -514 | Bindable, RequestEdit |
| `Font` | Get/Set | `Font` | -512 | Bindable |
| `HotTracking` | Get/Let | `Boolean` | 16 | Bindable, RequestEdit |
| `hWnd` | Get/Let | `Integer` | -515 | NonBrowsable |
| `ImageList` | Get/Let/Set | `Object` | 13 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 5 | — |
| `MousePointer` | Get/Let | `MousePointerConstants` | 2 | — |
| `MultiRow` | Get/Let | `Boolean` | 1 | — |
| `MultiSelect` | Get/Let | `Boolean` | 17 | Bindable, RequestEdit |
| `OLEDropMode` | Get/Let | `OLEDropConstants` | 1551 | — |
| `Placement` | Get/Let | `PlacementConstants` | 18 | — |
| `SelectedItem` | Get/Set/Let | `ITab` | 15 | NonBrowsable |
| `Separators` | Get/Let | `Boolean` | 19 | Bindable, RequestEdit |
| `ShowTips` | Get/Let | `Boolean` | 3 | — |
| `Style` | Get/Let | `TabStyleConstants` | 6 | — |
| `TabFixedHeight` | Get/Let | `Integer` | 14 | — |
| `TabFixedWidth` | Get/Let | `Integer` | 7 | — |
| `TabMinWidth` | Get/Let | `Single` | 20 | — |
| `Tabs` | Get/Set | `ITabs` | 4 | — |
| `TabStyle` | Get/Let | `TabSelStyleConstants` | 21 | — |
| `TabWidthStyle` | Get/Let | `TabWidthStyleConstants` | 8 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Refresh` | `` | `Void` | -550 | — |
| `OLEDrag` | `` | `Void` | 1552 | — |
| `AboutBox` | `` | `Void` | -552 | Hidden |
| `DeselectAll` | `` | `Void` | 22 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Click` | `` | -600 |
| `KeyDown` | `KeyCode: Integer, Shift: Integer` | -602 |
| `KeyPress` | `KeyAscii: Integer` | -603 |
| `KeyUp` | `KeyCode: Integer, Shift: Integer` | -604 |
| `MouseDown` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -605 |
| `MouseMove` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -606 |
| `MouseUp` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -607 |
| `BeforeClick` | `Cancel: Integer` | 1 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 1550 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 1551 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 1552 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 1553 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single, ByRef State: Integer` | 1554 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single` | 1555 |

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- Tab collection creation, removal, lookup, selection, and multi-selection.
- Tab layout, sizing, placement, separators, styles, images, and tooltips.
- Keyboard, pointer, selection, and OLE drag-and-drop event behavior.
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
