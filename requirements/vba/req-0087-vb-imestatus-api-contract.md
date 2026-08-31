# REQ-0087 — VBA VbIMEStatus values

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies wherever VBA exposes the `VbIMEStatus` constants.

## Requirement

The WFC VBA runtime shall expose each listed constant with the specified name, value, and member identifier.

## Value Inventory

| Constant | Value | Member ID |
| --- | ---: | ---: |
| `vbIMENoOp` | `0` | 1073741824 |
| `vbIMEModeNoControl` | `0` | 1073741825 |
| `vbIMEOn` | `1` | 1073741826 |
| `vbIMEModeOn` | `1` | 1073741827 |
| `vbIMEOff` | `2` | 1073741828 |
| `vbIMEModeOff` | `2` | 1073741829 |
| `vbIMEDisable` | `3` | 1073741830 |
| `vbIMEModeDisable` | `3` | 1073741831 |
| `vbIMEHiragana` | `4` | 1073741832 |
| `vbIMEModeHiragana` | `4` | 1073741833 |
| `vbIMEKatakanaDbl` | `5` | 1073741834 |
| `vbIMEModeKatakana` | `5` | 1073741835 |
| `vbIMEKatakanaSng` | `6` | 1073741836 |
| `vbIMEModeKatakanaHalf` | `6` | 1073741837 |
| `vbIMEAlphaDbl` | `7` | 1073741838 |
| `vbIMEModeAlphaFull` | `7` | 1073741839 |
| `vbIMEAlphaSng` | `8` | 1073741840 |
| `vbIMEModeAlpha` | `8` | 1073741841 |
| `vbIMEModeHangulFull` | `9` | 1073741842 |
| `vbIMEModeHangul` | `10` | 1073741843 |

## Rationale

This contract is part of the source-visible VBA standard library required
to compile and execute compatible VB and VBA code.

## Verification

**Method:** Inspection

**References:** `evidence/reference/vba-6.0-type-library.json`; the constant unit tests verify the exposed members (`REQ-0179`).

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
