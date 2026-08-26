# REQ-0048 — VB Timer API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Timer` class on supported Windows targets.

## Requirement

The WFC `Timer` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F28-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Timer`, `{33AD4F29-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `TimerEvents`, `{33AD4F2A-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `Boolean` | 0 | — |
| `Enabled` | Get/Let | `Boolean` | 65538 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Interval` | Get/Let | `Long` | 65539 | — |
| `Name` | Get | `String` | 65536 | — |
| `Parent` | Get | `Form` | 65540 | — |
| `Tag` | Get/Let | `String` | 65541 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AddItem` | `: String, : Variant optional` | `HRESULT` | 131072 | Hidden |
| `RemoveItem` | `: Integer` | `HRESULT` | 131073 | Hidden |
| `Clear` | `` | `HRESULT` | 131074 | Hidden |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Timer` | `` | 262144 |

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
