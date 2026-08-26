# REQ-0041 — VB CommandButton API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `CommandButton` class on supported Windows targets.

## Requirement

The WFC `CommandButton` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4EF0-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_CommandButton`, `{33AD4EF1-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `CommandButtonEvents`, `{33AD4EF2-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `Boolean` | 0 | — |
| `Appearance` | Get/Let | `Integer` | 65567 | — |
| `BackColor` | Get/Let | `Long` | 65539 | — |
| `Cancel` | Get/Let | `Boolean` | 65556 | — |
| `Caption` | Get/Let | `String` | 65537 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65578 | — |
| `Container` | Get/Set | `Object` | 65568 | — |
| `Default` | Get/Let | `Boolean` | 65555 | — |
| `DisabledPicture` | Get/Let/Set | `Picture` | 65571 | — |
| `DownPicture` | Get/Let/Set | `Picture` | 65572 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65559 | — |
| `DragMode` | Get/Let | `Integer` | 65558 | — |
| `Enabled` | Get/Let | `Boolean` | 65544 | — |
| `Font` | Get/Set | `Font` | 65565 | — |
| `FontBold` | Get/Let | `Boolean` | 65549 | — |
| `FontItalic` | Get/Let | `Boolean` | 65550 | — |
| `FontName` | Get/Let | `String` | 65547 | — |
| `FontSize` | Get/Let | `Single` | 65548 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65551 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65552 | — |
| `Height` | Get/Let | `Single` | 65543 | — |
| `HelpContextID` | Get/Let | `Long` | 65563 | — |
| `hWnd` | Get | `Long` | 65562 | — |
| `Index` | Get | `Integer` | 65538 | — |
| `Left` | Get/Let | `Single` | 65540 | — |
| `MaskColor` | Get/Let | `Long` | 65575 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65564 | — |
| `MousePointer` | Get/Let | `Integer` | 65546 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65574 | — |
| `Parent` | Get | `Form` | 65557 | — |
| `Picture` | Get/Let/Set | `Picture` | 65570 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65569 | — |
| `Style` | Get | `Integer` | 65577 | — |
| `TabIndex` | Get/Let | `Integer` | 65553 | — |
| `TabStop` | Get/Let | `Boolean` | 65560 | — |
| `Tag` | Get/Let | `String` | 65561 | — |
| `ToolTipText` | Get/Let | `String` | 65573 | — |
| `Top` | Get/Let | `Single` | 65541 | — |
| `UseMaskColor` | Get/Let | `Boolean` | 65576 | — |
| `Value` | Get/Let | `Boolean` | 65554 | — |
| `Visible` | Get/Let | `Boolean` | 65545 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65566 | — |
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
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262145 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262146 |
| `GotFocus` | `` | 262147 |
| `KeyDown` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262148 |
| `KeyPress` | `ByRef KeyAscii: Integer` | 262149 |
| `KeyUp` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262150 |
| `LostFocus` | `` | 262151 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262152 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262153 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262154 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262155 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262156 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262157 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262158 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262159 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262160 |

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
