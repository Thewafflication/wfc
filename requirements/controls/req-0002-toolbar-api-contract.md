# REQ-0002 — Toolbar Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`Toolbar` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `Toolbar` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.Toolbar.2` |
| Reference CLSID | `{66833FE6-8583-11D1-B16A-00C0F0283628}` |
| Default interface | `IToolbar`, `{66833FE4-8583-11D1-B16A-00C0F0283628}` |
| Default event interface | `IToolbarEvents`, `{66833FE5-8583-11D1-B16A-00C0F0283628}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AllowCustomize` | Get/Let | `Boolean` | 2 | — |
| `Appearance` | Get/Let | `AppearanceConstants` | -520 | — |
| `BorderStyle` | Get/Let | `BorderStyleConstants` | -504 | Bindable, RequestEdit |
| `ButtonHeight` | Get/Let | `Single` | 8 | — |
| `Buttons` | Get/Set | `IButtons*` | 3 | — |
| `ButtonWidth` | Get/Let | `Single` | 9 | — |
| `Controls` | Get | `IControls*` | 15 | — |
| `DisabledImageList` | Get/Let/Set | `Object` | 17 | — |
| `Enabled` | Get/Let | `Boolean` | -514 | Bindable, RequestEdit |
| `HelpContextID` | Get/Let | `Long` | 13 | — |
| `HelpFile` | Get/Let | `String` | 14 | — |
| `HotImageList` | Get/Let/Set | `Object` | 18 | — |
| `hWnd` | Get/Let | `Integer` | -515 | NonBrowsable |
| `ImageList` | Get/Let/Set | `Object` | 5 | — |
| `MouseIcon` | Get/Let/Set | `Picture*` | 4 | — |
| `MousePointer` | Get/Let | `MousePointerConstants` | 1 | — |
| `OLEDropMode` | Get/Let | `OLEDropConstants` | 1551 | — |
| `ShowTips` | Get/Let | `Boolean` | 6 | — |
| `Style` | Get/Let | `ToolbarStyleConstants` | 16 | — |
| `TextAlignment` | Get/Let | `ToolbarTextAlignConstants` | 19 | — |
| `Wrappable` | Get/Let | `Boolean` | 7 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Refresh` | `` | `Void` | -550 | — |
| `Customize` | `` | `Void` | 10 | — |
| `SaveToolbar` | `Key: String, Subkey: String, Value: String` | `Void` | 11 | — |
| `RestoreToolbar` | `Key: String, Subkey: String, Value: String` | `Void` | 12 | — |
| `OLEDrag` | `` | `Void` | 1552 | — |
| `AboutBox` | `` | `Void` | -552 | Hidden |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `ButtonClick` | `Button: Button*` | 1 |
| `Change` | `` | 2 |
| `Click` | `` | -600 |
| `MouseDown` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -605 |
| `MouseMove` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -606 |
| `MouseUp` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -607 |
| `DblClick` | `` | -601 |
| `OLEStartDrag` | `ByRef Data: DataObject**, ByRef AllowedEffects: Long*` | 1550 |
| `OLEGiveFeedback` | `ByRef Effect: Long*, ByRef DefaultCursors: Boolean*` | 1551 |
| `OLESetData` | `ByRef Data: DataObject**, ByRef DataFormat: Integer*` | 1552 |
| `OLECompleteDrag` | `ByRef Effect: Long*` | 1553 |
| `OLEDragOver` | `ByRef Data: DataObject**, ByRef Effect: Long*, ByRef Button: Integer*, ByRef Shift: Integer*, ByRef x: Single*, ByRef y: Single*, ByRef State: Integer*` | 1554 |
| `OLEDragDrop` | `ByRef Data: DataObject**, ByRef Effect: Long*, ByRef Button: Integer*, ByRef Shift: Integer*, ByRef x: Single*, ByRef y: Single*` | 1555 |
| `ButtonMenuClick` | `ButtonMenu: ButtonMenu*` | 3 |
| `ButtonDropDown` | `Button: Button*` | 4 |

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- Button and button-menu collection management, layout, images, and styles.
- Customization plus toolbar state save and restore.
- Button, menu, dropdown, pointer, and OLE drag-and-drop event behavior.
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
