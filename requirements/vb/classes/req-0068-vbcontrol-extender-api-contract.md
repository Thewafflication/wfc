# REQ-0068 — VB VBControlExtender API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `VBControlExtender` class on supported Windows targets.

## Requirement

The WFC `VBControlExtender` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{164CBDD2-7321-11D1-A1E8-00A0C90F2731}` |
| Default interface | `_VBControlExtender`, `{164CBDD0-7321-11D1-A1E8-00A0C90F2731}` |
| Default event interface | `VBControlExtenderEvents`, `{164CBDD1-7321-11D1-A1E8-00A0C90F2731}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `SetFocus` | `` | `Void` | -2147401728 | — |
| `ZOrder` | `Position: Variant optional` | `Void` | -2147401727 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `Void` | -2147401726 | — |
| `Drag` | `Action: Variant optional` | `Void` | -2147401725 | — |
| `ShowWhatsThis` | `` | `Void` | -2147401724 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `GotFocus` | `` | 1610678272 |
| `LostFocus` | `` | 1610678273 |
| `DragDrop` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single` | 1610678278 |
| `DragOver` | `ByRef Source: Control, ByRef X: Single, ByRef Y: Single, ByRef State: Integer` | 1610678279 |
| `Validate` | `ByRef Cancel: Boolean` | 1610678280 |
| `ObjectEvent` | `ByRef Info: EventInfo` | 1610678281 |

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
