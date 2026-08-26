# REQ-0062 — VB Data API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Data` class on supported Windows targets.

## Requirement

The WFC `Data` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4FF8-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Data`, `{33AD4FF9-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `DataEvents`, `{33AD4FFA-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Align` | Get/Let | `Integer` | 65570 | — |
| `Appearance` | Get/Let | `Integer` | 65575 | — |
| `BackColor` | Get/Let | `Long` | 65564 | — |
| `BOFAction` | Get/Let | `Integer` | 65571 | — |
| `Caption` | Get/Let | `String` | 65561 | — |
| `Connect` | Get/Let | `String` | 65554 | — |
| `Database` | Get | `Database` | 65556 | — |
| `DatabaseName` | Get/Let | `String` | 65555 | — |
| `DefaultCursorType` | Get/Let | `Integer` | 65581 | — |
| `DefaultType` | Get/Let | `Integer` | 65580 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65567 | — |
| `DragMode` | Get/Let | `Integer` | 65566 | — |
| `EditMode` | Get | `Integer` | 65562 | — |
| `Enabled` | Get/Let | `Boolean` | 65542 | — |
| `EOFAction` | Get/Let | `Integer` | 65572 | — |
| `Exclusive` | Get/Let | `Boolean` | 65558 | — |
| `Font` | Get/Set | `Font` | 65569 | — |
| `FontBold` | Get/Let | `Boolean` | 65547 | — |
| `FontItalic` | Get/Let | `Boolean` | 65548 | — |
| `FontName` | Get/Let | `String` | 65545 | — |
| `FontSize` | Get/Let | `Single` | 65546 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65549 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65550 | — |
| `ForeColor` | Get/Let | `Long` | 65565 | — |
| `Height` | Get/Let | `Single` | 65541 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Left` | Get/Let | `Single` | 65538 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65568 | — |
| `MousePointer` | Get/Let | `Integer` | 65544 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65578 | — |
| `Options` | Get/Let | `Integer` | 65563 | — |
| `Parent` | Get | `Form` | 65551 | — |
| `ReadOnly` | Get/Let | `Boolean` | 65559 | — |
| `Recordset` | Get/Set | `Recordset` | 65557 | — |
| `RecordsetType` | Get/Let | `Integer` | 65573 | — |
| `RecordSource` | Get/Let | `String` | 65560 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65577 | — |
| `Tag` | Get/Let | `String` | 65552 | — |
| `ToolTipText` | Get/Let | `String` | 65579 | — |
| `Top` | Get/Let | `Single` | 65539 | — |
| `Visible` | Get/Let | `Boolean` | 65543 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65574 | — |
| `Width` | Get/Let | `Single` | 65540 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Refresh` | `` | `HRESULT` | 131072 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131073 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131074 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131075 | — |
| `UpdateControls` | `` | `HRESULT` | 131076 | — |
| `UpdateRecord` | `` | `HRESULT` | 131077 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131078 | — |
| `OLEDrag` | `` | `HRESULT` | 131079 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Validate` | `ByRef Action: Integer, ByRef Save: Integer` | 262146 |
| `Error` | `ByRef DataErr: Integer, ByRef Response: Integer` | 262144 |
| `Reposition` | `` | 262145 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262147 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262148 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262149 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262150 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262151 |
| `Resize` | `` | 262152 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262153 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262154 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262155 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262156 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262157 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262158 |

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
