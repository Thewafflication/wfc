# REQ-0056 — VB Menu API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Menu` class on supported Windows targets.

## Requirement

The WFC `Menu` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F68-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Menu`, `{33AD4F69-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | `MenuEvents`, `{33AD4F6A-6699-11CF-B70C-00AA0060D393}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `_Default` | Get/Let | `Boolean` | 0 | — |
| `Caption` | Get/Let | `String` | 65539 | — |
| `Checked` | Get/Let | `Boolean` | 65540 | — |
| `Enabled` | Get/Let | `Boolean` | 65541 | — |
| `HelpContextID` | Get/Let | `Long` | 65548 | — |
| `Index` | Get | `Integer` | 65537 | — |
| `Name` | Get | `String` | 65536 | — |
| `Parent` | Get | `Form` | 65545 | — |
| `Tag` | Get/Let | `String` | 65546 | — |
| `Visible` | Get/Let | `Boolean` | 65538 | — |
| `WindowList` | Get | `Boolean` | 65547 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `Click` | `` | 262144 |

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
