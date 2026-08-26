# REQ-0040 — VB Frame API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Frame` class on supported Windows targets.

## Requirement

The WFC `Frame` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4EE8-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Frame`, `{33AD4EE9-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `FrameEvents`, `{33AD4EEA-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Appearance` | Get/Let | `Integer` | 65565 | — |
| `BackColor` | Get/Let | `Long` | 65539 | — |
| `BorderStyle` | Get/Let | `Integer` | 65570 | — |
| `Caption` | Get/Let | `String` | 65537 | — |
| `ClipControls` | Get/Let | `Boolean` | 65560 | — |
| `Container` | Get/Set | `Object` | 65566 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65557 | — |
| `DragMode` | Get/Let | `Integer` | 65556 | — |
| `Enabled` | Get/Let | `Boolean` | 65545 | — |
| `Font` | Get/Set | `Font` | 65563 | — |
| `FontBold` | Get/Let | `Boolean` | 65550 | — |
| `FontItalic` | Get/Let | `Boolean` | 65551 | — |
| `FontName` | Get/Let | `String` | 65548 | — |
| `FontSize` | Get/Let | `Single` | 65549 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65552 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65553 | — |
| `ForeColor` | Get/Let | `Long` | 65540 | — |
| `Height` | Get/Let | `Single` | 65544 | — |
| `HelpContextID` | Get/Let | `Long` | 65561 | — |
| `hWnd` | Get | `Long` | 65559 | — |
| `Index` | Get | `Integer` | 65538 | — |
| `Left` | Get/Let | `Single` | 65541 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65562 | — |
| `MousePointer` | Get/Let | `Integer` | 65547 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65569 | — |
| `Parent` | Get | `Form` | 65555 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65567 | — |
| `TabIndex` | Get/Let | `Integer` | 65554 | — |
| `Tag` | Get/Let | `String` | 65558 | — |
| `ToolTipText` | Get/Let | `String` | 65568 | — |
| `Top` | Get/Let | `Single` | 65542 | — |
| `Visible` | Get/Let | `Boolean` | 65546 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65564 | — |
| `Width` | Get/Let | `Single` | 65543 | — |

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
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262144 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262145 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262146 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262147 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262148 |
| `Click` | `` | 262149 |
| `DblClick` | `` | 262150 |
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
