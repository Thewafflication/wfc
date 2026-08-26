# REQ-0053 — VB DriveListBox API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `DriveListBox` class on supported Windows targets.

## Requirement

The WFC `DriveListBox` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F50-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_DriveListBox`, `{33AD4F51-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `DriveListBoxEvents`, `{33AD4F52-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Appearance` | Get/Let | `Integer` | 65568 | — |
| `BackColor` | Get/Let | `Long` | 65538 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65573 | — |
| `Container` | Get/Set | `Object` | 65569 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65560 | — |
| `DragMode` | Get/Let | `Integer` | 65559 | — |
| `Drive` | Get/Let | `String` | 65548 | — |
| `Enabled` | Get/Let | `Boolean` | 65544 | — |
| `Font` | Get/Set | `Font` | 65566 | — |
| `FontBold` | Get/Let | `Boolean` | 65554 | — |
| `FontItalic` | Get/Let | `Boolean` | 65555 | — |
| `FontName` | Get/Let | `String` | 65552 | — |
| `FontSize` | Get/Let | `Single` | 65553 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65556 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65557 | — |
| `ForeColor` | Get/Let | `Long` | 65539 | — |
| `Height` | Get/Let | `Single` | 65543 | — |
| `HelpContextID` | Get/Let | `Long` | 65564 | — |
| `hWnd` | Get | `Long` | 65563 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Left` | Get/Let | `Single` | 65540 | — |
| `List(: Integer)` | Get | `String` | 65551 | — |
| `ListCount` | Get | `Integer` | 65549 | — |
| `ListIndex` | Get/Let | `Integer` | 65550 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65565 | — |
| `MousePointer` | Get/Let | `Integer` | 65546 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65571 | — |
| `Parent` | Get | `Form` | 65558 | — |
| `TabIndex` | Get/Let | `Integer` | 65547 | — |
| `TabStop` | Get/Let | `Boolean` | 65561 | — |
| `Tag` | Get/Let | `String` | 65562 | — |
| `ToolTipText` | Get/Let | `String` | 65570 | — |
| `Top` | Get/Let | `Single` | 65541 | — |
| `TopIndex` | Get/Let | `Integer` | 65572 | — |
| `Visible` | Get/Let | `Boolean` | 65545 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65567 | — |
| `Width` | Get/Let | `Single` | 65542 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AddItem` | `: String, : Variant optional` | `HRESULT` | 131072 | Hidden |
| `RemoveItem` | `: Integer` | `HRESULT` | 131073 | Hidden |
| `Clear` | `` | `HRESULT` | 131074 | Hidden |
| `SetFocus` | `` | `HRESULT` | 131075 | — |
| `OLEDrag` | `` | `HRESULT` | 131076 | — |
| `Refresh` | `` | `HRESULT` | 131077 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131078 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131079 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131080 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131081 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Change` | `` | 262144 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262145 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262146 |
| `GotFocus` | `` | 262147 |
| `KeyDown` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262148 |
| `KeyPress` | `ByRef KeyAscii: Integer` | 262149 |
| `KeyUp` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262150 |
| `LostFocus` | `` | 262151 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262152 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262153 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262154 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262155 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262156 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262157 |
| `Scroll` | `` | 262158 |
| `Validate` | `ByRef Cancel: Boolean` | 262159 |

## Rationale

The VB object library defines the source-visible form, intrinsic-control, and
run-time service classes required to load and execute VB6 form projects.

## Verification

**Method:** Inspection

**References:** `evidence/reference/vb-6.0-type-library.json`; test case TBD

Inspect the WFC type information and compare every listed member name, DISPID,
access mode, signature, and event. Behavioral compatibility requires separate
requirements and executable VB6 reference tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** WCRT integration and form-runtime architecture decisions
- **Conflicts with:** None known

## Tailoring

A member may be deferred only through an approved compatibility-scope change
that identifies the resulting form or source-compatibility limitation.

## Implementation Record

Not implemented.
