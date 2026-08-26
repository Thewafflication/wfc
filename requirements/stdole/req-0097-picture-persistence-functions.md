# REQ-0097 — stdole picture persistence functions

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed OLE Automation standard type library (`stdole2.tlb`)

## Scope

This requirement applies to VB-visible standard picture loading and saving.

## Requirement

WFC shall expose `LoadPicture` and `SavePicture` with the names, signatures,
optional arguments, defaults, return types, and member identifiers below.
The implementation shall preserve reference file-format selection, requested
size conversion, palette-mode flags, error reporting, and picture identity.

## Function Inventory

| Member | Kind | Parameters | Returns | Member ID |
| --- | --- | --- | --- | ---: |
| `LoadPicture` | Method | `filename: Variant optional, widthDesired: Integer optional = 0, heightDesired: Integer optional = 0, flags: LoadPictureConstants optional = 0` | `Picture` | 1610612736 |
| `SavePicture` | Method | `Picture: Picture, filename: String` | `HRESULT` | 1610612737 |

## Rationale

VB6 forms and intrinsic controls exchange standard OLE font and picture
objects. Preserving these contracts is required for source, Automation,
persistence, and binary interoperability.

## Verification

**Method:** Inspection and test

**References:** `evidence/reference/stdole-2.0-type-library.json`; test case TBD

Compare type information and use fixtures covering empty input, missing and malformed files, each supported picture format, requested dimensions, flags, save/reload equivalence, and failure codes.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** COM and OLE Automation compatibility architecture
- **Conflicts with:** None known

## Tailoring

A listed interface or member may be deferred only through an approved
compatibility-scope change identifying the affected VB6 features.

## Implementation Record

Not implemented.
