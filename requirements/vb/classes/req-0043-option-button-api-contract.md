# REQ-0043 — VB OptionButton API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `OptionButton` class on supported Windows targets.

## Requirement

The WFC `OptionButton` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F00-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_OptionButton`, `{33AD4F01-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `OptionButtonEvents`, `{33AD4F02-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `Boolean` | 0 | — |
| `Alignment` | Get/Let | `Integer` | 65561 | — |
| `Appearance` | Get/Let | `Integer` | 65567 | — |
| `BackColor` | Get/Let | `Long` | 65539 | — |
| `Caption` | Get/Let | `String` | 65537 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65578 | — |
| `Container` | Get/Set | `Object` | 65568 | — |
| `DisabledPicture` | Get/Let/Set | `Picture` | 65572 | — |
| `DownPicture` | Get/Let/Set | `Picture` | 65573 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65558 | — |
| `DragMode` | Get/Let | `Integer` | 65557 | — |
| `Enabled` | Get/Let | `Boolean` | 65545 | — |
| `Font` | Get/Set | `Font` | 65565 | — |
| `FontBold` | Get/Let | `Boolean` | 65550 | — |
| `FontItalic` | Get/Let | `Boolean` | 65551 | — |
| `FontName` | Get/Let | `String` | 65548 | — |
| `FontSize` | Get/Let | `Single` | 65549 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65552 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65553 | — |
| `ForeColor` | Get/Let | `Long` | 65540 | — |
| `Height` | Get/Let | `Single` | 65544 | — |
| `HelpContextID` | Get/Let | `Long` | 65562 | — |
| `hWnd` | Get | `Long` | 65563 | — |
| `Index` | Get | `Integer` | 65538 | — |
| `Left` | Get/Let | `Single` | 65541 | — |
| `MaskColor` | Get/Let | `Long` | 65576 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65564 | — |
| `MousePointer` | Get/Let | `Integer` | 65547 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65575 | — |
| `Parent` | Get | `Form` | 65556 | — |
| `Picture` | Get/Let/Set | `Picture` | 65571 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65569 | — |
| `Style` | Get | `Integer` | 65574 | — |
| `TabIndex` | Get/Let | `Integer` | 65554 | — |
| `TabStop` | Get/Let | `Boolean` | 65559 | — |
| `Tag` | Get/Let | `String` | 65560 | — |
| `ToolTipText` | Get/Let | `String` | 65570 | — |
| `Top` | Get/Let | `Single` | 65542 | — |
| `UseMaskColor` | Get/Let | `Boolean` | 65577 | — |
| `Value` | Get/Let | `Boolean` | 65555 | — |
| `Visible` | Get/Let | `Boolean` | 65546 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65566 | — |
| `Width` | Get/Let | `Single` | 65543 | — |

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
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262156 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262157 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262158 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262159 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262160 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262161 |
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
