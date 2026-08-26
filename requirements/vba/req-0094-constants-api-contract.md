# REQ-0094 — VBA Constants values

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies wherever VBA exposes the `Constants` constants.

## Requirement

The WFC VBA runtime shall expose each listed constant with the specified name, value, and member identifier.

## Value Inventory

| Constant | Value | Member ID |
| --- | ---: | ---: |
| `vbObjectError` | `-2147221504` | 1073741824 |
| `vbNullString` | `""` | 1073741825 |
| `vbNullChar` | `"\0"` | 1073741826 |
| `vbCrLf` | `"\r\n"` | 1073741827 |
| `vbNewLine` | `"\r\n"` | 1073741828 |
| `vbCr` | `"\r"` | 1073741829 |
| `vbLf` | `"\n"` | 1073741830 |
| `vbBack` | `"\b"` | 1073741831 |
| `vbFormFeed` | `"\f"` | 1073741832 |
| `vbTab` | `"\t"` | 1073741833 |
| `vbVerticalTab` | `"\v"` | 1073741834 |

## Rationale

This contract is part of the source-visible VBA standard library required
to compile and execute compatible VB and VBA code.

## Verification

**Method:** Inspection

**References:** `evidence/reference/vba-6.0-type-library.json`; test case TBD

Inspect the WFC VBA type information and compare every listed name, value,
member identifier, access mode, and signature. Function behavior requires
separate executable reference and conformance tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** VBA constant-expression and enumeration support
- **Conflicts with:** None known

## Tailoring

A member or value may be deferred only through an approved compatibility-scope
change that identifies the resulting source or run-time limitation.

## Implementation Record

Not implemented.
