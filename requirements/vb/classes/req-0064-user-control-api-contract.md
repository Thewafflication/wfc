# REQ-0064 — VB UserControl API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `UserControl` class on supported Windows targets.

## Requirement

The WFC `UserControl` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD5010-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_UserControl`, `{33AD5011-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `UserControlEvents`, `{33AD5012-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get | `Object` | 0 | — |
| `AccessKeys` | Get/Let | `String` | 65627 | — |
| `ActiveControl` | Get | `Control` | 65588 | — |
| `Ambient` | Get | `AmbientProperties` | 65613 | — |
| `Appearance` | Get/Let | `Integer` | 65601 | — |
| `AutoRedraw` | Get/Let | `Boolean` | 65568 | — |
| `BackColor` | Get/Let | `Long` | 65539 | — |
| `BackStyle` | Get/Let | `Integer` | 65623 | — |
| `BorderStyle` | Get/Let | `Integer` | 65625 | — |
| `ClipBehavior` | Get/Let | `Integer` | 65599 | — |
| `ClipControls` | Get/Let | `Boolean` | 65586 | — |
| `ContainedControls` | Get | `ContainedControls` | 65616 | — |
| `ContainerHwnd` | Get | `Long` | 65604 | — |
| `Controls` | Get | `Object` | 65594 | — |
| `Count` | Get | `Integer` | 65593 | — |
| `CurrentX` | Get/Let | `Single` | 65555 | — |
| `CurrentY` | Get/Let | `Single` | 65556 | — |
| `DataMembers` | Get | `DataMembers` | 65596 | — |
| `DrawMode` | Get/Let | `Integer` | 65567 | — |
| `DrawStyle` | Get/Let | `Integer` | 65563 | — |
| `DrawWidth` | Get/Let | `Integer` | 65564 | — |
| `Enabled` | Get/Let | `Boolean` | 65545 | — |
| `EventsFrozen` | Get | `Boolean` | 65624 | — |
| `Extender` | Get | `Object` | 65615 | — |
| `FillColor` | Get/Let | `Long` | 65566 | — |
| `FillStyle` | Get/Let | `Integer` | 65565 | — |
| `Font` | Get/Set | `Font` | 65600 | — |
| `FontBold` | Get/Let | `Boolean` | 65550 | — |
| `FontItalic` | Get/Let | `Boolean` | 65551 | — |
| `FontName` | Get/Let | `String` | 65548 | — |
| `FontSize` | Get/Let | `Single` | 65549 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65552 | — |
| `FontTransparent` | Get/Let | `Boolean` | 65562 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65553 | — |
| `ForeColor` | Get/Let | `Long` | 65540 | — |
| `HasDC` | Get | `Boolean` | 65606 | — |
| `hDC` | Get | `Long` | 65554 | — |
| `Height` | Get/Let | `Single` | 65544 | — |
| `HitBehavior` | Get/Let | `Integer` | 65610 | — |
| `hWnd` | Get | `Long` | 65538 | — |
| `HyperLink` | Get | `Hyperlink` | 65629 | — |
| `Image` | Get | `Picture` | 65577 | — |
| `KeyPreview` | Get/Let | `Boolean` | 65585 | — |
| `MaskColor` | Get/Let | `Long` | 65579 | — |
| `MaskPicture` | Get/Let/Set | `Picture` | 65580 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65595 | — |
| `MousePointer` | Get/Let | `Integer` | 65547 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65607 | — |
| `Palette` | Get/Let/Set | `Picture` | 65608 | — |
| `PaletteMode` | Get/Let | `Integer` | 65609 | — |
| `Parent` | Get | `Object` | 65578 | — |
| `ParentControls` | Get | `ParentControls` | 65617 | — |
| `Picture` | Get/Let/Set | `Picture` | 65569 | — |
| `PropertyPages(: Integer)` | Get/Let | `String` | 65614 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65605 | — |
| `ScaleHeight` | Get/Let | `Single` | 65560 | — |
| `ScaleLeft` | Get/Let | `Single` | 65557 | — |
| `ScaleMode` | Get/Let | `Integer` | 65561 | — |
| `ScaleTop` | Get/Let | `Single` | 65558 | — |
| `ScaleWidth` | Get/Let | `Single` | 65559 | — |
| `Tag` | Get/Let | `String` | 65583 | — |
| `Width` | Get/Let | `Single` | 65543 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Refresh` | `` | `HRESULT` | 131072 | — |
| `SetFocus` | `` | `HRESULT` | 131074 | — |
| `DataMemberChanged` | `DataMember: String` | `HRESULT` | 131078 | — |
| `PopupMenu` | `Menu: Object, Flags: Variant optional, X: Variant optional, Y: Variant optional, DefaultMenu: Variant optional` | `HRESULT` | 131079 | — |
| `Circle` | `Step: Integer, X: Single, Y: Single, Radius: Single, Color: Long, Start: Single, End: Single, Aspect: Single` | `HRESULT` | 131080 | — |
| `Cls` | `` | `HRESULT` | 131081 | — |
| `Line` | `Flags: Integer, X1: Single, Y1: Single, X2: Single, Y2: Single, Color: Long` | `HRESULT` | 131082 | — |
| `PaintPicture` | `Picture: Picture, X1: Single, Y1: Single, Width1: Variant optional, Height1: Variant optional, X2: Variant optional, Y2: Variant optional, Width2: Variant optional, Height2: Variant optional, Opcode: Variant optional` | `HRESULT` | 131083 | — |
| `Point` | `X: Single, Y: Single` | `Long` | 131084 | — |
| `PSet` | `Step: Integer, X: Single, Y: Single, Color: Long` | `HRESULT` | 131085 | — |
| `Scale` | `Flags: Integer, X1: Variant optional, Y1: Variant optional, X2: Variant optional, Y2: Variant optional` | `HRESULT` | 131086 | — |
| `ScaleX` | `Width: Single, FromScale: Variant optional, ToScale: Variant optional` | `Single` | 131087 | — |
| `ScaleY` | `Height: Single, FromScale: Variant optional, ToScale: Variant optional` | `Single` | 131088 | — |
| `TextWidth` | `Str: String` | `Single` | 131089 | — |
| `TextHeight` | `Str: String` | `Single` | 131090 | — |
| `ValidateControls` | `` | `HRESULT` | 131092 | — |
| `OLEDrag` | `` | `HRESULT` | 131093 | — |
| `PropertyChanged` | `PropertyName: Variant optional` | `HRESULT` | 131094 | — |
| `CanPropertyChange` | `PropertyName: String` | `Boolean` | 131095 | — |
| `Size` | `Width: Single, Height: Single` | `HRESULT` | 131096 | — |
| `AsyncRead` | `Target: String, AsyncType: Long, PropertyName: Variant optional, AsyncReadOptions: Variant optional` | `HRESULT` | 131097 | — |
| `CancelAsyncRead` | `Property: Variant optional` | `HRESULT` | 131098 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Initialize` | `` | -6 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262144 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262145 |
| `Resize` | `` | 262151 |
| `Click` | `` | 262156 |
| `DblClick` | `` | 262157 |
| `GotFocus` | `` | 262158 |
| `KeyDown` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262159 |
| `KeyPress` | `ByRef KeyAscii: Integer` | 262160 |
| `KeyUp` | `ByRef KeyCode: Integer, ByRef Shift: Integer` | 262161 |
| `LostFocus` | `` | 262162 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262163 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262164 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262165 |
| `Paint` | `` | 262166 |
| `Terminate` | `` | -7 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262169 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262170 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262171 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262172 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262173 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262174 |
| `WriteProperties` | `ByRef PropBag: _PropertyBag` | 262175 |
| `ReadProperties` | `ByRef PropBag: _PropertyBag` | 262176 |
| `InitProperties` | `` | 262177 |
| `AsyncReadComplete` | `ByRef AsyncProp: AsyncProperty` | 262178 |
| `EnterFocus` | `` | 262179 |
| `ExitFocus` | `` | 262180 |
| `Show` | `` | 262181 |
| `Hide` | `` | 262182 |
| `AmbientChanged` | `ByRef PropertyName: String` | 262183 |
| `AccessKeyPress` | `ByRef KeyAscii: Integer` | 262184 |
| `AsyncReadProgress` | `ByRef AsyncProp: AsyncProperty` | 262185 |
| `GetDataMember` | `ByRef DataMember: String, ByRef Data: Object` | 262186 |
| `HitTest` | `ByRef X: Single, ByRef Y: Single, ByRef HitResult: Integer` | 262187 |

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
