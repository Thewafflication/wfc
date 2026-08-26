# REQ-0063 — VB OLE API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `OLE` class on supported Windows targets.

## Requirement

The WFC `OLE` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD5000-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_OLE`, `{33AD5001-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `OLEEvents`, `{33AD5002-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Let | `Integer` | 0 | — |
| `Action` | Let | `Integer` | 65579 | — |
| `Appearance` | Get/Let | `Integer` | 65608 | — |
| `AppIsRunning` | Get/Let | `Boolean` | 65562 | — |
| `AutoActivate` | Get/Let | `Integer` | 65560 | — |
| `AutoVerbMenu` | Get/Let | `Boolean` | 65561 | — |
| `BackColor` | Get/Let | `Long` | 65538 | — |
| `BackStyle` | Get/Let | `Integer` | 65589 | — |
| `BorderStyle` | Get/Let | `Integer` | 65548 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65583 | — |
| `Class` | Get/Let | `String` | 65553 | — |
| `Container` | Get/Set | `Object` | 65607 | — |
| `Data` | Get/Let | `Long` | 65565 | — |
| `DataChanged` | Get/Let | `Boolean` | 65601 | — |
| `DataField` | Get/Let | `String` | 65602 | — |
| `DataText` | Get/Let | `String` | 65566 | — |
| `DisplayType` | Get/Let | `Integer` | 65587 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65546 | — |
| `DragMode` | Get/Let | `Integer` | 65545 | — |
| `Enabled` | Get/Let | `Boolean` | 65549 | — |
| `FileNumber` | Get/Let | `Integer` | 65578 | — |
| `Format` | Get/Let | `String` | 65567 | — |
| `Height` | Get/Let | `Single` | 65542 | — |
| `HelpContextID` | Get/Let | `Long` | 65586 | — |
| `HostName` | Get/Let | `String` | 65558 | — |
| `hWnd` | Get | `Long` | 65585 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Left` | Get/Let | `Single` | 65539 | — |
| `LpOleObject` | Get | `Long` | 65588 | — |
| `MiscFlags` | Get/Let | `Integer` | 65580 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65590 | — |
| `MousePointer` | Get/Let | `Integer` | 65552 | — |
| `Name` | Get | `String` | 65536 | — |
| `object` | Get | `Object` | 65564 | — |
| `ObjectAcceptFormats(: Integer)` | Get | `String` | 65568 | — |
| `ObjectAcceptFormatsCount` | Get | `Integer` | 65569 | — |
| `ObjectGetFormats(: Integer)` | Get | `String` | 65570 | — |
| `ObjectGetFormatsCount` | Get | `Integer` | 65571 | — |
| `ObjectVerbFlags(: Integer)` | Get | `Long` | 65575 | — |
| `ObjectVerbs(: Integer)` | Get | `String` | 65574 | — |
| `ObjectVerbsCount` | Get | `Integer` | 65576 | — |
| `OLEDropAllowed` | Get/Let | `Boolean` | 65605 | — |
| `OLEType` | Get | `Integer` | 65554 | — |
| `OLETypeAllowed` | Get/Let | `Integer` | 65572 | — |
| `Parent` | Get | `Form` | 65544 | — |
| `PasteOK` | Get | `Boolean` | 65573 | — |
| `Picture` | Get | `Picture` | 65563 | — |
| `SizeMode` | Get/Let | `Integer` | 65559 | — |
| `SourceDoc` | Get/Let | `String` | 65555 | — |
| `SourceItem` | Get/Let | `String` | 65556 | — |
| `TabIndex` | Get/Let | `Integer` | 65551 | — |
| `TabStop` | Get/Let | `Boolean` | 65550 | — |
| `Tag` | Get/Let | `String` | 65547 | — |
| `Top` | Get/Let | `Single` | 65540 | — |
| `UpdateOptions` | Get/Let | `Integer` | 65557 | — |
| `Verb` | Get/Let | `Integer` | 65577 | — |
| `Visible` | Get/Let | `Boolean` | 65543 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65606 | — |
| `Width` | Get/Let | `Single` | 65541 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `SetFocus` | `` | `HRESULT` | 131072 | — |
| `Refresh` | `` | `HRESULT` | 131073 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131074 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131075 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131076 | — |
| `CreateEmbed` | `SourceDoc: String, Class: Variant optional` | `HRESULT` | 131077 | — |
| `CreateLink` | `SourceDoc: String, SourceItem: Variant optional` | `HRESULT` | 131078 | — |
| `Copy` | `` | `HRESULT` | 131079 | — |
| `Paste` | `` | `HRESULT` | 131080 | — |
| `Update` | `` | `HRESULT` | 131081 | — |
| `DoVerb` | `Verb: Variant optional` | `HRESULT` | 131082 | — |
| `FetchVerbs` | `` | `HRESULT` | 131083 | — |
| `Close` | `` | `HRESULT` | 131084 | — |
| `Delete` | `` | `HRESULT` | 131085 | — |
| `SaveToFile` | `FileNum: Integer` | `HRESULT` | 131086 | — |
| `SaveToOle1File` | `FileNum: Integer` | `HRESULT` | 131087 | — |
| `ReadFromFile` | `FileNum: Integer` | `HRESULT` | 131088 | — |
| `InsertObjDlg` | `` | `HRESULT` | 131089 | — |
| `PasteSpecialDlg` | `` | `HRESULT` | 131090 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131091 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Updated` | `ByRef Code: Integer` | 262157 |
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
| `Resize` | `ByRef HeightNew: Single, ByRef WidthNew: Single` | 262156 |
| `ObjectMove` | `ByRef Left: Single, ByRef Top: Single, ByRef Width: Single, ByRef Height: Single` | 262158 |
| `Validate` | `ByRef Cancel: Boolean` | 262159 |

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
