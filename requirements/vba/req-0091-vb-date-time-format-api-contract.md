# REQ-0091 — VBA VbDateTimeFormat values

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies wherever VBA exposes the `VbDateTimeFormat` constants.

## Requirement

The WFC VBA runtime shall expose each listed constant with the specified name, value, and member identifier.

## Value Inventory

| Constant | Value | Member ID |
| --- | ---: | ---: |
| `vbGeneralDate` | `0` | 1073741824 |
| `vbLongDate` | `1` | 1073741825 |
| `vbShortDate` | `2` | 1073741826 |
| `vbLongTime` | `3` | 1073741827 |
| `vbShortTime` | `4` | 1073741828 |

## Rationale

This contract is part of the source-visible VBA standard library required
to compile and execute compatible VB and VBA code.

## Verification

**Method:** Inspection

**References:** `evidence/reference/vba-6.0-type-library.json`; `TC-MP0002-vba-constants-cli` and the constant unit tests verify the exposed members (`REQ-0179`).

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
