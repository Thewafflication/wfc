# REQ-0080 — VBA VbVarType values

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies wherever VBA exposes the `VbVarType` constants.

## Requirement

The WFC VBA runtime shall expose each listed constant with the specified name, value, and member identifier.

## Value Inventory

| Constant | Value | Member ID |
| --- | ---: | ---: |
| `vbEmpty` | `0` | 1073741824 |
| `vbNull` | `1` | 1073741825 |
| `vbInteger` | `2` | 1073741826 |
| `vbLong` | `3` | 1073741827 |
| `vbSingle` | `4` | 1073741828 |
| `vbDouble` | `5` | 1073741829 |
| `vbCurrency` | `6` | 1073741830 |
| `vbDate` | `7` | 1073741831 |
| `vbString` | `8` | 1073741832 |
| `vbObject` | `9` | 1073741833 |
| `vbError` | `10` | 1073741834 |
| `vbBoolean` | `11` | 1073741835 |
| `vbVariant` | `12` | 1073741836 |
| `vbDataObject` | `13` | 1073741837 |
| `vbDecimal` | `14` | 1073741838 |
| `vbByte` | `17` | 1073741839 |
| `vbUserDefinedType` | `36` | 1073741840 |
| `vbArray` | `8192` | 1073741841 |

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
