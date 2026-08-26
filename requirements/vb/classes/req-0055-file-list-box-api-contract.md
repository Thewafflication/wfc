# REQ-0055 — VB FileListBox API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `FileListBox` class on supported Windows targets.

## Requirement

The WFC `FileListBox` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F60-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_FileListBox`, `{33AD4F61-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `FileListBoxEvents`, `{33AD4F62-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Appearance` | Get/Let | `Integer` | 65579 | — |
| `Archive` | Get/Let | `Boolean` | 65553 | — |
| `BackColor` | Get/Let | `Long` | 65538 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65584 | — |
| `Container` | Get/Set | `Object` | 65580 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65567 | — |
| `DragMode` | Get/Let | `Integer` | 65566 | — |
| `Enabled` | Get/Let | `Boolean` | 65544 | — |
| `FileName` | Get/Let | `String` | 65550 | — |
| `Font` | Get/Set | `Font` | 65577 | — |
| `FontBold` | Get/Let | `Boolean` | 65561 | — |
| `FontItalic` | Get/Let | `Boolean` | 65562 | — |
| `FontName` | Get/Let | `String` | 65559 | — |
| `FontSize` | Get/Let | `Single` | 65560 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65563 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65564 | — |
| `ForeColor` | Get/Let | `Long` | 65539 | — |
| `Height` | Get/Let | `Single` | 65543 | — |
| `HelpContextID` | Get/Let | `Long` | 65571 | — |
| `Hidden` | Get/Let | `Boolean` | 65554 | — |
| `hWnd` | Get | `Long` | 65570 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Left` | Get/Let | `Single` | 65540 | — |
| `List(: Integer)` | Get | `String` | 65558 | — |
| `ListCount` | Get | `Integer` | 65556 | — |
| `ListIndex` | Get/Let | `Integer` | 65557 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65576 | — |
| `MousePointer` | Get/Let | `Integer` | 65546 | — |
| `MultiSelect` | Get | `Integer` | 65572 | — |
| `Name` | Get | `String` | 65536 | — |
| `Normal` | Get/Let | `Boolean` | 65551 | — |
| `OLEDragMode` | Get/Let | `Integer` | 65582 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65583 | — |
| `Parent` | Get | `Form` | 65565 | — |
| `Path` | Get/Let | `String` | 65548 | — |
| `Pattern` | Get/Let | `String` | 65549 | — |
| `ReadOnly` | Get/Let | `Boolean` | 65552 | — |
| `Selected(: Integer)` | Get/Let | `Boolean` | 65573 | — |
| `System` | Get/Let | `Boolean` | 65555 | — |
| `TabIndex` | Get/Let | `Integer` | 65547 | — |
| `TabStop` | Get/Let | `Boolean` | 65568 | — |
| `Tag` | Get/Let | `String` | 65569 | — |
| `ToolTipText` | Get/Let | `String` | 65581 | — |
| `Top` | Get/Let | `Single` | 65541 | — |
| `TopIndex` | Get/Let | `Integer` | 65574 | — |
| `Visible` | Get/Let | `Boolean` | 65545 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65578 | — |
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
| `Click` | `` | 262144 |
| `DblClick` | `` | 262145 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262146 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262147 |
| `GotFocus` | `` | 262148 |
| `KeyDown` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262149 |
| `KeyPress` | `ByRef KeyAscii: Integer` | 262150 |
| `KeyUp` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262151 |
| `LostFocus` | `` | 262152 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262153 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262154 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262155 |
| `PathChange` | `` | 262156 |
| `PatternChange` | `` | 262157 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262158 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262159 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262160 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262161 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262162 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262163 |
| `Scroll` | `` | 262164 |
| `Validate` | `ByRef Cancel: Boolean` | 262165 |

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
