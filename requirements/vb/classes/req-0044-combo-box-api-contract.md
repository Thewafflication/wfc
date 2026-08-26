# REQ-0044 — VB ComboBox API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `ComboBox` class on supported Windows targets.

## Requirement

The WFC `ComboBox` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F08-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_ComboBox`, `{33AD4F09-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `ComboBoxEvents`, `{33AD4F0A-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Appearance` | Get/Let | `Integer` | 65579 | — |
| `BackColor` | Get/Let | `Long` | 65539 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65588 | — |
| `Container` | Get/Set | `Object` | 65580 | — |
| `DataChanged` | Get/Let | `Boolean` | 65577 | — |
| `DataField` | Get/Let | `String` | 65576 | — |
| `DataFormat` | Get/Set | `IStdDataFormatDisp` | 65590 | — |
| `DataMember` | Get/Let | `String` | 65589 | — |
| `DataSource` | Get/Set | `DataSource` | 65591 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65565 | — |
| `DragMode` | Get/Let | `Integer` | 65564 | — |
| `Enabled` | Get/Let | `Boolean` | 65545 | — |
| `Font` | Get/Set | `Font` | 65574 | — |
| `FontBold` | Get/Let | `Boolean` | 65550 | — |
| `FontItalic` | Get/Let | `Boolean` | 65551 | — |
| `FontName` | Get/Let | `String` | 65549 | — |
| `FontSize` | Get/Let | `Single` | 65554 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65552 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65553 | — |
| `ForeColor` | Get/Let | `Long` | 65540 | — |
| `Height` | Get/Let | `Single` | 65544 | — |
| `HelpContextID` | Get/Let | `Long` | 65571 | — |
| `hWnd` | Get | `Long` | 65568 | — |
| `Index` | Get | `Integer` | 65538 | — |
| `IntegralHeight` | Get | `Boolean` | 65581 | — |
| `ItemData(: Integer)` | Get/Let | `Long` | 65569 | — |
| `Left` | Get/Let | `Single` | 65541 | — |
| `List(: Integer)` | Get/Let | `String` | 65558 | — |
| `ListCount` | Get | `Integer` | 65556 | — |
| `ListIndex` | Get/Let | `Integer` | 65557 | — |
| `Locked` | Get/Let | `Boolean` | 65586 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65573 | — |
| `MousePointer` | Get/Let | `Integer` | 65547 | — |
| `Name` | Get | `String` | 65536 | — |
| `NewIndex` | Get | `Integer` | 65570 | — |
| `OLEDragMode` | Get/Let | `Integer` | 65584 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65585 | — |
| `Parent` | Get | `Form` | 65563 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65582 | — |
| `SelLength` | Get/Let | `Long` | 65561 | — |
| `SelStart` | Get/Let | `Long` | 65560 | — |
| `SelText` | Get/Let | `String` | 65562 | — |
| `Sorted` | Get | `Boolean` | 65559 | — |
| `Style` | Get | `Integer` | 65537 | — |
| `TabIndex` | Get/Let | `Integer` | 65555 | — |
| `TabStop` | Get/Let | `Boolean` | 65566 | — |
| `Tag` | Get/Let | `String` | 65567 | — |
| `Text` | Get/Let | `String` | 65548 | — |
| `ToolTipText` | Get/Let | `String` | 65583 | — |
| `Top` | Get/Let | `Single` | 65542 | — |
| `TopIndex` | Get/Let | `Integer` | 65587 | — |
| `Visible` | Get/Let | `Boolean` | 65546 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65578 | — |
| `Width` | Get/Let | `Single` | 65543 | — |

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
| `Change` | `` | 262144 |
| `Click` | `` | 262145 |
| `DblClick` | `` | 262146 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262147 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262148 |
| `DropDown` | `` | 262149 |
| `GotFocus` | `` | 262150 |
| `KeyDown` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262151 |
| `KeyPress` | `ByRef KeyAscii: Integer` | 262152 |
| `KeyUp` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262153 |
| `LostFocus` | `` | 262154 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262155 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262156 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262157 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262158 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262159 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262160 |
| `Scroll` | `` | 262161 |
| `Validate` | `ByRef Cancel: Boolean` | 262162 |

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
