# REQ-0038 — VB Label API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Label` class on supported Windows targets.

## Requirement

The WFC `Label` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4ED8-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Label`, `{33AD4ED9-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `LabelEvents`, `{33AD4EDA-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `String` | 0 | — |
| `Alignment` | Get/Let | `Integer` | 65556 | — |
| `Appearance` | Get/Let | `Integer` | 65575 | — |
| `AutoSize` | Get/Let | `Boolean` | 65560 | — |
| `BackColor` | Get/Let | `Long` | 65539 | — |
| `BackStyle` | Get/Let | `Integer` | 65567 | — |
| `BorderStyle` | Get/Let | `Integer` | 65555 | — |
| `Caption` | Get/Let | `String` | 65537 | — |
| `Container` | Get/Set | `Object` | 65576 | — |
| `DataChanged` | Get/Let | `Boolean` | 65570 | — |
| `DataField` | Get/Let | `String` | 65569 | — |
| `DataFormat` | Get/Set | `IStdDataFormatDisp` | 65581 | — |
| `DataMember` | Get/Let | `String` | 65580 | — |
| `DataSource` | Get/Set | `DataSource` | 65582 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65563 | — |
| `DragMode` | Get/Let | `Integer` | 65562 | — |
| `Enabled` | Get/Let | `Boolean` | 65545 | — |
| `Font` | Get/Set | `Font` | 65573 | — |
| `FontBold` | Get/Let | `Boolean` | 65550 | — |
| `FontItalic` | Get/Let | `Boolean` | 65551 | — |
| `FontName` | Get/Let | `String` | 65548 | — |
| `FontSize` | Get/Let | `Single` | 65549 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65552 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65553 | — |
| `ForeColor` | Get/Let | `Long` | 65540 | — |
| `Height` | Get/Let | `Single` | 65544 | — |
| `Index` | Get | `Integer` | 65538 | — |
| `Left` | Get/Let | `Single` | 65541 | — |
| `LinkItem` | Get/Let | `String` | 65558 | — |
| `LinkMode` | Get/Let | `Integer` | 65559 | — |
| `LinkTimeout` | Get/Let | `Integer` | 65564 | — |
| `LinkTopic` | Get/Let | `String` | 65557 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65571 | — |
| `MousePointer` | Get/Let | `Integer` | 65547 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65579 | — |
| `Parent` | Get | `Form` | 65561 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65577 | — |
| `TabIndex` | Get/Let | `Integer` | 65554 | — |
| `Tag` | Get/Let | `String` | 65565 | — |
| `ToolTipText` | Get/Let | `String` | 65578 | — |
| `Top` | Get/Let | `Single` | 65542 | — |
| `UseMnemonic` | Get/Let | `Boolean` | 65572 | — |
| `Visible` | Get/Let | `Boolean` | 65546 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65574 | — |
| `Width` | Get/Let | `Single` | 65543 | — |
| `WordWrap` | Get/Let | `Boolean` | 65566 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AddItem` | `: String, : Variant optional` | `HRESULT` | 131072 | Hidden |
| `RemoveItem` | `: Integer` | `HRESULT` | 131073 | Hidden |
| `Clear` | `` | `HRESULT` | 131074 | Hidden |
| `LinkExecute` | `Command: String` | `HRESULT` | 131075 | — |
| `LinkPoke` | `` | `HRESULT` | 131076 | — |
| `LinkRequest` | `` | `HRESULT` | 131077 | — |
| `LinkSend` | `` | `HRESULT` | 131078 | — |
| `OLEDrag` | `` | `HRESULT` | 131079 | — |
| `Refresh` | `` | `HRESULT` | 131080 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131081 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131082 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131083 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131084 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Click` | `` | 262145 |
| `Change` | `` | 262144 |
| `DblClick` | `` | 262146 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262147 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262148 |
| `LinkClose` | `` | 262149 |
| `LinkError` | `ByRef LinkErr: Integer` | 262150 |
| `LinkOpen` | `ByRef Cancel: Integer` | 262151 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262152 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262153 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262154 |
| `LinkNotify` | `` | 262155 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262156 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262157 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262158 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262159 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262160 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262161 |

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
