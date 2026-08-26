# REQ-0057 — VB MDIForm API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `MDIForm` class on supported Windows targets.

## Requirement

The WFC `MDIForm` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F70-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_MDIForm`, `{33AD4F71-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `MDIFormEvents`, `{33AD4F72-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get | `Object` | 0 | — |
| `ActiveControl` | Get | `Control` | 65588 | — |
| `ActiveForm` | Get | `Object` | 65549 | — |
| `Appearance` | Get/Let | `Integer` | 65601 | — |
| `AutoShowChildren` | Get/Let | `Boolean` | 65596 | — |
| `BackColor` | Get/Let | `Long` | 65539 | — |
| `Caption` | Get/Let | `String` | 65537 | — |
| `Controls` | Get | `Object` | 65594 | — |
| `Count` | Get | `Integer` | 65593 | — |
| `Enabled` | Get/Let | `Boolean` | 65545 | — |
| `Height` | Get/Let | `Single` | 65544 | — |
| `HelpContextID` | Get/Let | `Long` | 65587 | — |
| `hWnd` | Get | `Long` | 65538 | — |
| `Icon` | Get/Let/Set | `Picture` | 65571 | — |
| `Left` | Get/Let | `Single` | 65541 | — |
| `LinkMode` | Get/Let | `Integer` | 65573 | — |
| `LinkTopic` | Get/Let | `String` | 65572 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65595 | — |
| `MousePointer` | Get/Let | `Integer` | 65547 | — |
| `Moveable` | Get | `Boolean` | 65610 | — |
| `Name` | Get | `String` | 65536 | — |
| `OLEDropMode` | Get/Let | `Integer` | 65607 | — |
| `Picture` | Get/Let/Set | `Picture` | 65569 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65605 | — |
| `ScaleHeight` | Get | `Single` | 65560 | — |
| `ScaleWidth` | Get | `Single` | 65559 | — |
| `ScrollBars` | Get | `Boolean` | 65548 | — |
| `StartUpPosition` | Get | `Integer` | 65606 | — |
| `Tag` | Get/Let | `String` | 65583 | — |
| `Top` | Get/Let | `Single` | 65542 | — |
| `Visible` | Get/Let | `Boolean` | 65582 | — |
| `WhatsThisHelp` | Get | `Boolean` | 65603 | — |
| `Width` | Get/Let | `Single` | 65543 | — |
| `WindowState` | Get/Let | `Integer` | 65546 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131073 | — |
| `SetFocus` | `` | `HRESULT` | 131074 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131075 | — |
| `Show` | `Modal: Variant optional, OwnerForm: Variant optional` | `HRESULT` | 131076 | — |
| `Hide` | `` | `HRESULT` | 131077 | — |
| `PopupMenu` | `Menu: Object, Flags: Variant optional, X: Variant optional, Y: Variant optional, DefaultMenu: Variant optional` | `HRESULT` | 131079 | — |
| `ValidateControls` | `` | `HRESULT` | 131090 | — |
| `Arrange` | `Arrangement: Integer` | `HRESULT` | 131091 | — |
| `WhatsThisMode` | `` | `HRESULT` | 131092 | — |
| `OLEDrag` | `` | `HRESULT` | 131093 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Load` | `` | 262150 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 262144 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262145 |
| `LinkClose` | `` | 262146 |
| `LinkError` | `ByRef LinkErr: Integer` | 262147 |
| `LinkExecute` | `ByRef CmdStr: String, ByRef Cancel: Integer` | 262148 |
| `LinkOpen` | `ByRef Cancel: Integer` | 262149 |
| `Resize` | `` | 262151 |
| `Unload` | `ByRef Cancel: Integer` | 262152 |
| `QueryUnload` | `ByRef Cancel: Integer, ByRef UnloadMode: Integer` | 262153 |
| `Activate` | `` | 262154 |
| `Deactivate` | `` | 262155 |
| `Click` | `` | 262156 |
| `DblClick` | `` | 262157 |
| `MouseDown` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262163 |
| `MouseMove` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262164 |
| `MouseUp` | `ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262165 |
| `Initialize` | `` | -6 |
| `Terminate` | `` | -7 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 262169 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef X: Single, ByRef Y: Single` | 262170 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 262171 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 262172 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 262173 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 262174 |

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
