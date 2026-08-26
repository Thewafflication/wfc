# REQ-0045 — VB ListBox API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `ListBox` class on supported Windows targets.

## Requirement

The WFC `ListBox` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F10-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_ListBox`, `{33AD4F11-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `ListBoxEvents`, `{33AD4F12-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Appearance` | Get/Let | `Integer` | 65580 | — |
| `BackColor` | Get/Let | `Long` | 65538 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65588 | — |
| `Columns` | Get/Let | `Integer` | 65564 | — |
| `Container` | Get/Set | `Object` | 65581 | — |
| `DataChanged` | Get/Let | `Boolean` | 65578 | — |
| `DataField` | Get/Let | `String` | 65577 | — |
| `DataFormat` | Get/Set | `IStdDataFormatDisp` | 65590 | — |
| `DataMember` | Get/Let | `String` | 65589 | — |
| `DataSource` | Get/Set | `DataSource` | 65591 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65561 | — |
| `DragMode` | Get/Let | `Integer` | 65560 | — |
| `Enabled` | Get/Let | `Boolean` | 65544 | — |
| `Font` | Get/Set | `Font` | 65575 | — |
| `FontBold` | Get/Let | `Boolean` | 65549 | — |
| `FontItalic` | Get/Let | `Boolean` | 65550 | — |
| `FontName` | Get/Let | `String` | 65547 | — |
| `FontSize` | Get/Let | `Single` | 65548 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65551 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65552 | — |
| `ForeColor` | Get/Let | `Long` | 65539 | — |
| `Height` | Get/Let | `Single` | 65543 | — |
| `HelpContextID` | Get/Let | `Long` | 65571 | — |
| `hWnd` | Get | `Long` | 65572 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `IntegralHeight` | Get | `Boolean` | 65582 | — |
| `ItemData(: Integer)` | Get/Let | `Long` | 65569 | — |
| `Left` | Get/Let | `Single` | 65540 | — |
| `List(: Integer)` | Get/Let | `String` | 65556 | — |
| `ListCount` | Get | `Integer` | 65554 | — |
| `ListIndex` | Get/Let | `Integer` | 65555 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65574 | — |
| `MousePointer` | Get/Let | `Integer` | 65546 | — |
| `MultiSelect` | Get | `Integer` | 65565 | — |
| `Name` | Get | `String` | 65536 | — |
| `NewIndex` | Get | `Integer` | 65570 | — |
| `OLEDragMode` | Get/Let | `Integer` | 65585 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65586 | — |
| `Parent` | Get | `Form` | 65559 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65583 | — |
| `SelCount` | Get | `Integer` | 65567 | — |
| `Selected(: Integer)` | Get/Let | `Boolean` | 65566 | — |
| `Sorted` | Get | `Boolean` | 65557 | — |
| `Style` | Get | `Integer` | 65587 | — |
| `TabIndex` | Get/Let | `Integer` | 65553 | — |
| `TabStop` | Get/Let | `Boolean` | 65562 | — |
| `Tag` | Get/Let | `String` | 65563 | — |
| `Text` | Get/Let | `String` | 65558 | — |
| `ToolTipText` | Get/Let | `String` | 65584 | — |
| `Top` | Get/Let | `Single` | 65541 | — |
| `TopIndex` | Get/Let | `Integer` | 65568 | — |
| `Visible` | Get/Let | `Boolean` | 65545 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65579 | — |
| `Width` | Get/Let | `Single` | 65542 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Clear` | `` | `HRESULT` | 131072 | — |
| `AddItem` | `Item: String, Index: Variant optional` | `HRESULT` | 131073 | — |
| `RemoveItem` | `Index: Integer` | `HRESULT` | 131074 | — |
| `SetFocus` | `` | `HRESULT` | 131075 | — |
| `Refresh` | `` | `HRESULT` | 131076 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131077 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131078 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131079 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131080 | — |
| `OLEDrag` | `` | `HRESULT` | 131081 | — |

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
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262156 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262157 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262158 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262159 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262160 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262161 |
| `Scroll` | `` | 262162 |
| `ItemCheck` | `ByRef Item: Integer` | 262163 |
| `Validate` | `ByRef Cancel: Boolean` | 262164 |

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
