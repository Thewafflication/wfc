# REQ-0039 — VB TextBox API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `TextBox` class on supported Windows targets.

## Requirement

The WFC `TextBox` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4EE0-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_TextBox`, `{33AD4EE1-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `TextBoxEvents`, `{33AD4EE2-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Alignment` | Get/Let | `Integer` | 65572 | — |
| `Appearance` | Get/Let | `Integer` | 65584 | — |
| `BackColor` | Get/Let | `Long` | 65538 | — |
| `BorderStyle` | Get/Let | `Integer` | 65555 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65590 | — |
| `Container` | Get/Set | `Object` | 65585 | — |
| `DataChanged` | Get/Let | `Boolean` | 65579 | — |
| `DataField` | Get/Let | `String` | 65578 | — |
| `DataFormat` | Get/Set | `IStdDataFormatDisp` | 65592 | — |
| `DataMember` | Get/Let | `String` | 65591 | — |
| `DataSource` | Get/Set | `DataSource` | 65593 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65566 | — |
| `DragMode` | Get/Let | `Integer` | 65565 | — |
| `Enabled` | Get/Let | `Boolean` | 65544 | — |
| `Font` | Get/Set | `Font` | 65582 | — |
| `FontBold` | Get/Let | `Boolean` | 65550 | — |
| `FontItalic` | Get/Let | `Boolean` | 65551 | — |
| `FontName` | Get/Let | `String` | 65548 | — |
| `FontSize` | Get/Let | `Single` | 65549 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65552 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65553 | — |
| `ForeColor` | Get/Let | `Long` | 65539 | — |
| `Height` | Get/Let | `Single` | 65543 | — |
| `HelpContextID` | Get/Let | `Long` | 65574 | — |
| `HideSelection` | Get | `Boolean` | 65571 | — |
| `hWnd` | Get | `Long` | 65575 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Left` | Get/Let | `Single` | 65540 | — |
| `LinkItem` | Get/Let | `String` | 65557 | — |
| `LinkMode` | Get/Let | `Integer` | 65558 | — |
| `LinkTimeout` | Get/Let | `Integer` | 65567 | — |
| `LinkTopic` | Get/Let | `String` | 65556 | — |
| `Locked` | Get/Let | `Boolean` | 65581 | — |
| `MaxLength` | Get/Let | `Long` | 65573 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65580 | — |
| `MousePointer` | Get/Let | `Integer` | 65546 | — |
| `MultiLine` | Get | `Boolean` | 65559 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDragMode` | Get/Let | `Integer` | 65588 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65589 | — |
| `Parent` | Get | `Form` | 65564 | — |
| `PasswordChar` | Get/Let | `String` | 65570 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65586 | — |
| `ScrollBars` | Get | `Integer` | 65560 | — |
| `SelLength` | Get/Let | `Long` | 65562 | — |
| `SelStart` | Get/Let | `Long` | 65561 | — |
| `SelText` | Get/Let | `String` | 65563 | — |
| `TabIndex` | Get/Let | `Integer` | 65554 | — |
| `TabStop` | Get/Let | `Boolean` | 65568 | — |
| `Tag` | Get/Let | `String` | 65569 | — |
| `Text` | Get/Let | `String` | 65547 | — |
| `ToolTipText` | Get/Let | `String` | 65587 | — |
| `Top` | Get/Let | `Single` | 65541 | — |
| `Visible` | Get/Let | `Boolean` | 65545 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65583 | — |
| `Width` | Get/Let | `Single` | 65542 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AddItem` | `: String, : Variant optional` | `HRESULT` | 131072 | Hidden |
| `RemoveItem` | `: Integer` | `HRESULT` | 131073 | Hidden |
| `Clear` | `` | `HRESULT` | 131074 | Hidden |
| `SetFocus` | `` | `HRESULT` | 131075 | — |
| `LinkExecute` | `Command: String` | `HRESULT` | 131076 | — |
| `LinkPoke` | `` | `HRESULT` | 131077 | — |
| `LinkRequest` | `` | `HRESULT` | 131078 | — |
| `LinkSend` | `` | `HRESULT` | 131079 | — |
| `OLEDrag` | `` | `HRESULT` | 131080 | — |
| `Refresh` | `` | `HRESULT` | 131081 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131082 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131083 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131084 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131085 | — |

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
| `LinkClose` | `` | 262151 |
| `LinkError` | `ByRef LinkErr: Integer` | 262152 |
| `LinkOpen` | `ByRef Cancel: Integer` | 262153 |
| `LostFocus` | `` | 262154 |
| `LinkNotify` | `` | 262155 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262156 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262157 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262158 |
| `Click` | `` | 262159 |
| `DblClick` | `` | 262160 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262161 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262162 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262163 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262164 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262165 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262166 |
| `Validate` | `ByRef Cancel: Boolean` | 262167 |

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
