# REQ-0067 — VB Global API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Global` class on supported Windows targets.

## Requirement

The WFC `Global` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{FCFB3D23-A0FA-1068-A738-08002B3371B5}` |
| Default interface | `VBGlobal`, `{FCFB3D22-A0FA-1068-A738-08002B3371B5}` |
| Default event interface | None exposed |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `App` | Get | `App` | 1610678274 | — |
| `Clipboard` | Get | `Clipboard` | 1610678276 | — |
| `Forms` | Get | `Object` | 1610678279 | — |
| `Licenses` | Get | `Licenses` | 1610678288 | — |
| `Printer` | Get/Set | `Printer` | 1610678277 | — |
| `Printers` | Get | `Object` | 1610678280 | — |
| `Screen` | Get | `Screen` | 1610678275 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Load` | `object: Object` | `HRESULT` | 1610678272 | — |
| `Unload` | `object: Object` | `HRESULT` | 1610678273 | — |
| `LoadResPicture` | `id: Variant, restype: Integer` | `Picture` | 1610678282 | — |
| `LoadResData` | `id: Variant, type: Variant` | `Variant` | 1610678283 | — |
| `SavePicture` | `Picture: Picture, FileName: String` | `HRESULT` | 1610678285 | — |
| `LoadPicture` | `FileName: Variant optional, Size: Variant optional, ColorDepth: Variant optional, X: Variant optional, Y: Variant optional` | `Picture` | 1610678286 | — |
| `LoadResString` | `id: Long` | `String` | 1610678287 | — |

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
