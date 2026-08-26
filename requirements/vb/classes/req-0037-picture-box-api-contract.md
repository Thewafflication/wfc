# REQ-0037 — VB PictureBox API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `PictureBox` class on supported Windows targets.

## Requirement

The WFC `PictureBox` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4ED0-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_PictureBox`, `{33AD4ED1-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `PictureBoxEvents`, `{33AD4ED2-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let/Set | `Variant` | 0 | — |
| `Align` | Get/Let | `Integer` | 65586 | — |
| `Appearance` | Get/Let | `Integer` | 65595 | — |
| `AutoRedraw` | Get/Let | `Boolean` | 65569 | — |
| `AutoSize` | Get/Let | `Boolean` | 65571 | — |
| `BackColor` | Get/Let | `Long` | 65537 | — |
| `BorderStyle` | Get/Let | `Integer` | 65572 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65570 | — |
| `ClipControls` | Get/Let | `Boolean` | 65584 | — |
| `Container` | Get/Set | `Object` | 65596 | — |
| `CurrentX` | Get/Let | `Single` | 65556 | — |
| `CurrentY` | Get/Let | `Single` | 65557 | — |
| `DataChanged` | Get/Let | `Boolean` | 65590 | — |
| `DataField` | Get/Let | `String` | 65589 | — |
| `DataFormat` | Get/Set | `IStdDataFormatDisp` | 65602 | — |
| `DataMember` | Get/Let | `String` | 65601 | — |
| `DataSource` | Get/Set | `DataSource` | 65603 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65579 | — |
| `DragMode` | Get/Let | `Integer` | 65578 | — |
| `DrawMode` | Get/Let | `Integer` | 65568 | — |
| `DrawStyle` | Get/Let | `Integer` | 65564 | — |
| `DrawWidth` | Get/Let | `Integer` | 65565 | — |
| `Enabled` | Get/Let | `Boolean` | 65545 | — |
| `FillColor` | Get/Let | `Long` | 65567 | — |
| `FillStyle` | Get/Let | `Integer` | 65566 | — |
| `Font` | Get/Set | `Font` | 65593 | — |
| `FontBold` | Get/Let | `Boolean` | 65550 | — |
| `FontItalic` | Get/Let | `Boolean` | 65551 | — |
| `FontName` | Get/Let | `String` | 65548 | — |
| `FontSize` | Get/Let | `Single` | 65549 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65552 | — |
| `FontTransparent` | Get/Let | `Boolean` | 65563 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65553 | — |
| `ForeColor` | Get/Let | `Long` | 65540 | — |
| `HasDC` | Get | `Boolean` | 65604 | — |
| `hDC` | Get | `Long` | 65555 | — |
| `Height` | Get/Let | `Single` | 65544 | — |
| `HelpContextID` | Get/Let | `Long` | 65585 | — |
| `hWnd` | Get | `Long` | 65583 | — |
| `Image` | Get | `Picture` | 65576 | — |
| `Index` | Get | `Integer` | 65538 | — |
| `Left` | Get/Let | `Single` | 65541 | — |
| `LinkItem` | Get/Let | `String` | 65574 | — |
| `LinkMode` | Get/Let | `Integer` | 65575 | — |
| `LinkTimeout` | Get/Let | `Integer` | 65580 | — |
| `LinkTopic` | Get/Let | `String` | 65573 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65591 | — |
| `MousePointer` | Get/Let | `Integer` | 65547 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDragMode` | Get/Let | `Integer` | 65599 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65600 | — |
| `Parent` | Get | `Form` | 65577 | — |
| `Picture` | Get/Let/Set | `Picture` | 65539 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65597 | — |
| `ScaleHeight` | Get/Let | `Single` | 65561 | — |
| `ScaleLeft` | Get/Let | `Single` | 65558 | — |
| `ScaleMode` | Get/Let | `Integer` | 65562 | — |
| `ScaleTop` | Get/Let | `Single` | 65559 | — |
| `ScaleWidth` | Get/Let | `Single` | 65560 | — |
| `TabIndex` | Get/Let | `Integer` | 65554 | — |
| `TabStop` | Get/Let | `Boolean` | 65581 | — |
| `Tag` | Get/Let | `String` | 65582 | — |
| `ToolTipText` | Get/Let | `String` | 65598 | — |
| `Top` | Get/Let | `Single` | 65542 | — |
| `Visible` | Get/Let | `Boolean` | 65546 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65594 | — |
| `Width` | Get/Let | `Single` | 65543 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131072 | — |
| `LinkExecute` | `Command: String` | `HRESULT` | 131073 | — |
| `LinkPoke` | `` | `HRESULT` | 131074 | — |
| `LinkRequest` | `` | `HRESULT` | 131075 | — |
| `LinkSend` | `` | `HRESULT` | 131076 | — |
| `Refresh` | `` | `HRESULT` | 131077 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131078 | — |
| `SetFocus` | `` | `HRESULT` | 131079 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131080 | — |
| `Circle` | `Step: Integer, X: Single, Y: Single, Radius: Single, Color: Long, Start: Single, End: Single, Aspect: Single` | `HRESULT` | 131081 | — |
| `Cls` | `` | `HRESULT` | 131082 | — |
| `Line` | `Flags: Integer, X1: Single, Y1: Single, X2: Single, Y2: Single, Color: Long` | `HRESULT` | 131083 | — |
| `PaintPicture` | `Picture: Picture, X1: Single, Y1: Single, Width1: Variant optional, Height1: Variant optional, X2: Variant optional, Y2: Variant optional, Width2: Variant optional, Height2: Variant optional, Opcode: Variant optional` | `HRESULT` | 131084 | — |
| `Point` | `X: Single, Y: Single` | `Long` | 131085 | — |
| `PSet` | `Step: Integer, X: Single, Y: Single, Color: Long` | `HRESULT` | 131086 | — |
| `Scale` | `Flags: Integer, X1: Variant optional, Y1: Variant optional, X2: Variant optional, Y2: Variant optional` | `HRESULT` | 131087 | — |
| `ScaleX` | `Width: Single, FromScale: Variant optional, ToScale: Variant optional` | `Single` | 131088 | — |
| `ScaleY` | `Height: Single, FromScale: Variant optional, ToScale: Variant optional` | `Single` | 131089 | — |
| `TextWidth` | `Str: String` | `Single` | 131090 | — |
| `TextHeight` | `Str: String` | `Single` | 131091 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131092 | — |
| `OLEDrag` | `` | `HRESULT` | 131093 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Click` | `` | 262145 |
| `Change` | `` | 262144 |
| `DblClick` | `` | 262146 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262147 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262148 |
| `GotFocus` | `` | 262149 |
| `KeyDown` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262150 |
| `KeyPress` | `ByRef KeyAscii: Integer` | 262151 |
| `KeyUp` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262152 |
| `LinkClose` | `` | 262153 |
| `LinkError` | `ByRef LinkErr: Integer` | 262154 |
| `LinkOpen` | `ByRef Cancel: Integer` | 262155 |
| `LostFocus` | `` | 262156 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262157 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262158 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262159 |
| `Paint` | `` | 262160 |
| `LinkNotify` | `` | 262161 |
| `Resize` | `` | 262162 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262163 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262164 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262165 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262166 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262167 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262168 |
| `Validate` | `ByRef Cancel: Boolean` | 262169 |

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
