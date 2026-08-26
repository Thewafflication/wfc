# REQ-0007 — ImageList Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`ImageList` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `ImageList` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.ImageListCtrl.2` |
| Reference CLSID | `{2C247F23-8591-11D1-B16A-00C0F0283628}` |
| Default interface | `IImageList`, `{2C247F21-8591-11D1-B16A-00C0F0283628}` |
| Default event interface | None exposed |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `BackColor` | Get/Let | `ULong` | -501 | — |
| `hImageList` | Get/Let | `Integer` | 5 | NonBrowsable |
| `ImageHeight` | Get/Let | `Integer` | 1 | — |
| `ImageWidth` | Get/Let | `Integer` | 2 | — |
| `ListImages` | Get/Set | `IImages*` | 4 | — |
| `MaskColor` | Get/Let | `ULong` | 3 | — |
| `UseMaskColor` | Get/Let | `Boolean` | 7 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Overlay` | `Key1: Variant*, Key2: Variant*` | `Picture*` | 6 | — |
| `AboutBox` | `` | `Void` | -552 | Hidden |

## Event Inventory

The reference control exposes no members on its default event interface.

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- Image collection creation, removal, lookup, extraction, and replacement.
- Image dimensions, mask color, background color, overlay images, and native handle behavior.
- Picture format, transparency, scaling, indexing, and error semantics.
- Default values, boundary values, invalid inputs, error numbers, persistence,
  rendering, focus, keyboard navigation, and behavior at common DPI settings.

## Rationale

Matching the published Automation surface is necessary for source-level VB6
compatibility and provides a stable boundary for later behavioral probes.

## Verification

**Method:** Inspection

**References:** `evidence/reference/mscomctl-2.0-type-library.json`; test case TBD

Inspect the WFC type library and compare every listed member name, DISPID,
access mode, signature, and event against this requirement. Runtime semantics
are verified by separate behavioral requirements and tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** WCRT integration ADR and COM identity/coexistence ADR
- **Conflicts with:** None known

## Tailoring

A member may be deferred only through an approved compatibility-scope change
that identifies the resulting limitation.

## Implementation Record

Not implemented.
