# REQ-0061 — VB Image API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Image` class on supported Windows targets.

## Requirement

The WFC `Image` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F90-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Image`, `{33AD4F91-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `ImageEvents`, `{33AD4F92-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let/Set | `Variant` | 0 | — |
| `Appearance` | Get/Let | `Integer` | 65557 | — |
| `BorderStyle` | Get/Let | `Integer` | 65551 | — |
| `Container` | Get/Set | `Object` | 65558 | — |
| `DataChanged` | Get/Let | `Boolean` | 65554 | — |
| `DataField` | Get/Let | `String` | 65553 | — |
| `DataFormat` | Get/Set | `IStdDataFormatDisp` | 65563 | — |
| `DataMember` | Get/Let | `String` | 65562 | — |
| `DataSource` | Get/Set | `DataSource` | 65564 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65549 | — |
| `DragMode` | Get/Let | `Integer` | 65548 | — |
| `Enabled` | Get/Let | `Boolean` | 65543 | — |
| `Height` | Get/Let | `Single` | 65542 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Left` | Get/Let | `Single` | 65539 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65555 | — |
| `MousePointer` | Get/Let | `Integer` | 65545 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDragMode` | Get/Let | `Integer` | 65560 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65561 | — |
| `Parent` | Get | `Form` | 65547 | — |
| `Picture` | Get/Let/Set | `Picture` | 65538 | — |
| `Stretch` | Get/Let | `Boolean` | 65546 | — |
| `Tag` | Get/Let | `String` | 65550 | — |
| `ToolTipText` | Get/Let | `String` | 65559 | — |
| `Top` | Get/Let | `Single` | 65540 | — |
| `Visible` | Get/Let | `Boolean` | 65544 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65556 | — |
| `Width` | Get/Let | `Single` | 65541 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AddItem` | `: String, : Variant optional` | `HRESULT` | 131072 | Hidden |
| `RemoveItem` | `: Integer` | `HRESULT` | 131073 | Hidden |
| `Clear` | `` | `HRESULT` | 131074 | Hidden |
| `OLEDrag` | `` | `HRESULT` | 131075 | — |
| `Refresh` | `` | `HRESULT` | 131076 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131077 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131078 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131079 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131080 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Click` | `` | 262144 |
| `DblClick` | `` | 262145 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262146 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262147 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262148 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262149 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262150 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262151 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262152 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262153 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262154 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262155 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262156 |

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
