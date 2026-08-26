# REQ-0059 — VB Shape API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Shape` class on supported Windows targets.

## Requirement

The WFC `Shape` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F80-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Shape`, `{33AD4F81-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | None exposed |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `Integer` | 0 | — |
| `BackColor` | Get/Let | `Long` | 65538 | — |
| `BackStyle` | Get/Let | `Integer` | 65552 | — |
| `BorderColor` | Get/Let | `Long` | 65539 | — |
| `BorderStyle` | Get/Let | `Integer` | 65549 | — |
| `BorderWidth` | Get/Let | `Integer` | 65550 | — |
| `Container` | Get/Set | `Object` | 65554 | — |
| `DrawMode` | Get/Let | `Integer` | 65548 | — |
| `FillColor` | Get/Let | `Long` | 65551 | — |
| `FillStyle` | Get/Let | `Integer` | 65553 | — |
| `Height` | Get/Let | `Single` | 65543 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Left` | Get/Let | `Single` | 65540 | — |
| `Name` | Get | `String` | 65536 | — |
| `Parent` | Get | `Form` | 65545 | — |
| `Shape` | Get/Let | `Integer` | 65547 | — |
| `Tag` | Get/Let | `String` | 65546 | — |
| `Top` | Get/Let | `Single` | 65541 | — |
| `Visible` | Get/Let | `Boolean` | 65544 | — |
| `Width` | Get/Let | `Single` | 65542 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AddItem` | `: String, : Variant optional` | `HRESULT` | 131072 | Hidden |
| `RemoveItem` | `: Integer` | `HRESULT` | 131073 | Hidden |
| `Clear` | `` | `HRESULT` | 131074 | Hidden |
| `Refresh` | `` | `HRESULT` | 131075 | — |
| `ZOrder` | `Position: Variant optional` | `HRESULT` | 131076 | — |
| `Move` | `Left: Single, Top: Variant optional, Width: Variant optional, Height: Variant optional` | `HRESULT` | 131077 | — |
| `Drag` | `: Variant optional` | `HRESULT` | 131078 | Hidden |

## Event Inventory

The reference class exposes no members on its default event interface.

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
