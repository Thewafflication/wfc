# REQ-0051 — VB Screen API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Screen` class on supported Windows targets.

## Requirement

The WFC `Screen` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F40-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Screen`, `{33AD4F41-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | None exposed |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `ActiveControl` | Get | `Control` | 65541 | — |
| `ActiveForm` | Get | `Form` | 65540 | — |
| `FontCount` | Get | `Integer` | 65538 | — |
| `Fonts(: Integer)` | Get | `String` | 65539 | — |
| `Height` | Get | `Single` | 65537 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 65545 | — |
| `MousePointer` | Get/Let | `Integer` | 65542 | — |
| `TwipsPerPixelX` | Get | `Single` | 65543 | — |
| `TwipsPerPixelY` | Get | `Single` | 65544 | — |
| `Width` | Get | `Single` | 65546 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |

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
