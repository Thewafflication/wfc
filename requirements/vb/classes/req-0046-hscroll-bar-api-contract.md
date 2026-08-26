# REQ-0046 — VB HScrollBar API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `HScrollBar` class on supported Windows targets.

## Requirement

The WFC `HScrollBar` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F18-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_HScrollBar`, `{33AD4F19-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `HScrollBarEvents`, `{33AD4F1A-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `Integer` | 0 | — |
| `CausesValidation` | Get/Let | `Boolean` | 65562 | — |
| `Container` | Get/Set | `Object` | 65560 | — |
| `DragIcon` | Get/Let/Set | `Picture` | 65553 | — |
| `DragMode` | Get/Let | `Integer` | 65552 | — |
| `Enabled` | Get/Let | `Boolean` | 65542 | — |
| `Height` | Get/Let | `Single` | 65541 | — |
| `HelpContextID` | Get/Let | `Long` | 65557 | — |
| `hWnd` | Get | `Long` | 65556 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `LargeChange` | Get/Let | `Integer` | 65549 | — |
| `Left` | Get/Let | `Single` | 65538 | — |
| `Max` | Get/Let | `Integer` | 65547 | — |
| `Min` | Get/Let | `Integer` | 65546 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65558 | — |
| `MousePointer` | Get/Let | `Integer` | 65544 | — |
| `Name` | Get | `String` | 65536 | — |
| `Parent` | Get | `Form` | 65551 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65561 | — |
| `SmallChange` | Get/Let | `Integer` | 65548 | — |
| `TabIndex` | Get/Let | `Integer` | 65545 | — |
| `TabStop` | Get/Let | `Boolean` | 65554 | — |
| `Tag` | Get/Let | `String` | 65555 | — |
| `Top` | Get/Let | `Single` | 65539 | — |
| `Value` | Get/Let | `Integer` | 65550 | — |
| `Visible` | Get/Let | `Boolean` | 65543 | — |
| `WhatsThisHelpID` | Get/Let | `Long` | 65559 | — |
| `Width` | Get/Let | `Single` | 65540 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AddItem` | `: String, : Variant optional` | `HRESULT` | 131072 | Hidden |
| `RemoveItem` | `: Integer` | `HRESULT` | 131073 | Hidden |
| `Clear` | `` | `HRESULT` | 131074 | Hidden |
| `SetFocus` | `` | `HRESULT` | 131075 | — |
| `Refresh` | `` | `HRESULT` | 131076 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131077 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131078 | — |
| `Drag` | `Action: Variant optional` | `HRESULT` | 131079 | — |
| `ShowWhatsThis` | `` | `HRESULT` | 131080 | — |

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
| `LostFocus` | `` | 262151 |
| `Scroll` | `` | 262152 |
| `Validate` | `ByRef Cancel: Boolean` | 262153 |

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
